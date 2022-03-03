/*	$NetBSD: if_axe.c,v 1.142 2022/03/03 05:53:23 riastradh Exp $	*/
/*	$OpenBSD: if_axe.c,v 1.137 2016/04/13 11:03:37 mpi Exp $ */

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
 * ASIX Electronics AX88172/AX88178/AX88778 USB 2.0 ethernet driver.
 * Used in the LinkSys USB200M and various other adapters.
 *
 * Written by Bill Paul <wpaul@windriver.com>
 * Senior Engineer
 * Wind River Systems
 */

/*
 * The AX88172 provides USB ethernet supports at 10 and 100Mbps.
 * It uses an external PHY (reference designs use a RealTek chip),
 * and has a 64-bit multicast hash filter. There is some information
 * missing from the manual which one needs to know in order to make
 * the chip function:
 *
 * - You must set bit 7 in the RX control register, otherwise the
 *   chip won't receive any packets.
 * - You must initialize all 3 IPG registers, or you won't be able
 *   to send any packets.
 *
 * Note that this device appears to only support loading the station
 * address via autoload from the EEPROM (i.e. there's no way to manually
 * set it).
 *
 * (Adam Weinberger wanted me to name this driver if_gir.c.)
 */

/*
 * Ax88178 and Ax88772 support backported from the OpenBSD driver.
 * 2007/02/12, J.R. Oldroyd, fbsd@opal.com
 *
 * Manual here:
 * http://www.asix.com.tw/FrootAttach/datasheet/AX88178_datasheet_Rev10.pdf
 * http://www.asix.com.tw/FrootAttach/datasheet/AX88772_datasheet_Rev10.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_axe.c,v 1.142 2022/03/03 05:53:23 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/if_axereg.h>

struct axe_type {
	struct usb_devno	axe_dev;
	uint16_t		axe_flags;
};

struct axe_softc {
	struct usbnet		axe_un;

	/* usbnet:un_flags values */
#define AX178		__BIT(0)	/* AX88178 */
#define AX772		__BIT(1)	/* AX88772 */
#define AX772A		__BIT(2)	/* AX88772A */
#define AX772B		__BIT(3)	/* AX88772B */
#define	AXSTD_FRAME	__BIT(12)
#define	AXCSUM_FRAME	__BIT(13)

	uint8_t			axe_ipgs[3];
	uint8_t 		axe_phyaddrs[2];
	uint16_t		sc_pwrcfg;
	uint16_t		sc_lenmask;

};

#define AXE_IS_178_FAMILY(un)				\
	((un)->un_flags & (AX178 | AX772 | AX772A | AX772B))

#define AXE_IS_772(un)					\
	((un)->un_flags & (AX772 | AX772A | AX772B))

#define AXE_IS_172(un) (AXE_IS_178_FAMILY(un) == 0)

#define AX_RXCSUM					\
    (IFCAP_CSUM_IPv4_Rx | 				\
     IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |	\
     IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx)

#define AX_TXCSUM					\
    (IFCAP_CSUM_IPv4_Tx | 				\
     IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx |	\
     IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_UDPv6_Tx)

/*
 * AXE_178_MAX_FRAME_BURST
 * max frame burst size for Ax88178 and Ax88772
 *	0	2048 bytes
 *	1	4096 bytes
 *	2	8192 bytes
 *	3	16384 bytes
 * use the largest your system can handle without USB stalling.
 *
 * NB: 88772 parts appear to generate lots of input errors with
 * a 2K rx buffer and 8K is only slightly faster than 4K on an
 * EHCI port on a T42 so change at your own risk.
 */
#define AXE_178_MAX_FRAME_BURST	1


#ifdef USB_DEBUG
#ifndef AXE_DEBUG
#define axedebug 0
#else
static int axedebug = 0;

SYSCTL_SETUP(sysctl_hw_axe_setup, "sysctl hw.axe setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "axe",
	    SYSCTL_DESCR("axe global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &axedebug, sizeof(axedebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* AXE_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(axedebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(axedebug,N,FMT,A,B,C,D)
#define AXEHIST_FUNC()		USBHIST_FUNC()
#define AXEHIST_CALLED(name)	USBHIST_CALLED(axedebug)

/*
 * Various supported device vendors/products.
 */
static const struct axe_type axe_devs[] = {
	{ { USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_UFE2000 }, 0 },
	{ { USB_VENDOR_ACERCM,		USB_PRODUCT_ACERCM_EP1427X2 }, 0 },
	{ { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_ETHERNET }, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88172 }, 0 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772 }, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772A }, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772B }, AX772B },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772B_1 }, AX772B },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88178 }, AX178 },
	{ { USB_VENDOR_ATEN,		USB_PRODUCT_ATEN_UC210T }, 0 },
	{ { USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_F5D5055 }, AX178 },
	{ { USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USB2AR }, 0},
	{ { USB_VENDOR_CISCOLINKSYS,	USB_PRODUCT_CISCOLINKSYS_USB200MV2 }, AX772A },
	{ { USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB2_TX }, 0 },
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100 }, 0 },
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100B1 }, AX772 },
	{ { USB_VENDOR_DLINK2,		USB_PRODUCT_DLINK2_DUBE100B1 }, AX772 },
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100C1 }, AX772B },
	{ { USB_VENDOR_GOODWAY,		USB_PRODUCT_GOODWAY_GWUSB2E }, 0 },
	{ { USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_ETGUS2 }, AX178 },
	{ { USB_VENDOR_JVC,		USB_PRODUCT_JVC_MP_PRX1 }, 0 },
	{ { USB_VENDOR_LENOVO,		USB_PRODUCT_LENOVO_ETHERNET }, AX772B },
	{ { USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_HG20F9 }, AX772B },
	{ { USB_VENDOR_LINKSYS2,	USB_PRODUCT_LINKSYS2_USB200M }, 0 },
	{ { USB_VENDOR_LINKSYS4,	USB_PRODUCT_LINKSYS4_USB1000 }, AX178 },
	{ { USB_VENDOR_LOGITEC,		USB_PRODUCT_LOGITEC_LAN_GTJU2 }, AX178 },
	{ { USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUAU2GT }, AX178 },
	{ { USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUAU2KTX }, 0 },
	{ { USB_VENDOR_MSI,		USB_PRODUCT_MSI_AX88772A }, AX772 },
	{ { USB_VENDOR_NETGEAR,		USB_PRODUCT_NETGEAR_FA120 }, 0 },
	{ { USB_VENDOR_OQO,		USB_PRODUCT_OQO_ETHER01PLUS }, AX772 },
	{ { USB_VENDOR_PLANEX3,		USB_PRODUCT_PLANEX3_GU1000T }, AX178 },
	{ { USB_VENDOR_SITECOM,		USB_PRODUCT_SITECOM_LN029 }, 0 },
	{ { USB_VENDOR_SITECOMEU,	USB_PRODUCT_SITECOMEU_LN028 }, AX178 },
	{ { USB_VENDOR_SITECOMEU,	USB_PRODUCT_SITECOMEU_LN031 }, AX178 },
	{ { USB_VENDOR_SYSTEMTALKS,	USB_PRODUCT_SYSTEMTALKS_SGCX2UL }, 0 },
};
#define axe_lookup(v, p) ((const struct axe_type *)usb_lookup(axe_devs, v, p))

static const struct ax88772b_mfb ax88772b_mfb_table[] = {
	{ 0x8000, 0x8001, 2048 },
	{ 0x8100, 0x8147, 4096 },
	{ 0x8200, 0x81EB, 6144 },
	{ 0x8300, 0x83D7, 8192 },
	{ 0x8400, 0x851E, 16384 },
	{ 0x8500, 0x8666, 20480 },
	{ 0x8600, 0x87AE, 24576 },
	{ 0x8700, 0x8A3D, 32768 }
};

static int	axe_match(device_t, cfdata_t, void *);
static void	axe_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(axe, sizeof(struct axe_softc),
	axe_match, axe_attach, usbnet_detach, usbnet_activate);

static void	axe_uno_stop(struct ifnet *, int);
static void	axe_uno_mcast(struct ifnet *);
static int	axe_uno_init(struct ifnet *);
static int	axe_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int	axe_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void	axe_uno_mii_statchg(struct ifnet *);
static void	axe_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
				uint32_t);
static unsigned axe_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);

static void	axe_ax88178_init(struct axe_softc *);
static void	axe_ax88772_init(struct axe_softc *);
static void	axe_ax88772a_init(struct axe_softc *);
static void	axe_ax88772b_init(struct axe_softc *);

static const struct usbnet_ops axe_ops = {
	.uno_stop = axe_uno_stop,
	.uno_mcast = axe_uno_mcast,
	.uno_read_reg = axe_uno_mii_read_reg,
	.uno_write_reg = axe_uno_mii_write_reg,
	.uno_statchg = axe_uno_mii_statchg,
	.uno_tx_prepare = axe_uno_tx_prepare,
	.uno_rx_loop = axe_uno_rx_loop,
	.uno_init = axe_uno_init,
};

static usbd_status
axe_cmd(struct axe_softc *sc, int cmd, int index, int val, void *buf)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = &sc->axe_un;
	usb_device_request_t req;
	usbd_status err;

	if (usbnet_isdying(un))
		return -1;

	DPRINTFN(20, "cmd %#jx index %#jx val %#jx", cmd, index, val, 0);

	if (AXE_CMD_DIR(cmd))
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AXE_CMD_CMD(cmd);
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, AXE_CMD_LEN(cmd));

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err)
		DPRINTF("cmd %jd err %jd", cmd, err, 0, 0);

	return err;
}

static int
axe_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc * const sc = usbnet_softc(un);
	usbd_status err;
	uint16_t data;

	DPRINTFN(30, "phy %#jx reg %#jx\n", phy, reg, 0, 0);

	if (un->un_phyno != phy)
		return EINVAL;

	axe_cmd(sc, AXE_CMD_MII_OPMODE_SW, 0, 0, NULL);

	err = axe_cmd(sc, AXE_CMD_MII_READ_REG, reg, phy, &data);
	axe_cmd(sc, AXE_CMD_MII_OPMODE_HW, 0, 0, NULL);

	if (err) {
		device_printf(un->un_dev, "read PHY failed\n");
		return EIO;
	}

	*val = le16toh(data);
	if (AXE_IS_772(un) && reg == MII_BMSR) {
		/*
		 * BMSR of AX88772 indicates that it supports extended
		 * capability but the extended status register is
		 * reserved for embedded ethernet PHY. So clear the
		 * extended capability bit of BMSR.
		 */
		*val &= ~BMSR_EXTCAP;
	}

	DPRINTFN(30, "phy %#jx reg %#jx val %#jx", phy, reg, *val, 0);

	return 0;
}

static int
axe_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	struct axe_softc * const sc = usbnet_softc(un);
	usbd_status err;
	uint16_t aval;

	if (un->un_phyno != phy)
		return EINVAL;

	aval = htole16(val);

	axe_cmd(sc, AXE_CMD_MII_OPMODE_SW, 0, 0, NULL);
	err = axe_cmd(sc, AXE_CMD_MII_WRITE_REG, reg, phy, &aval);
	axe_cmd(sc, AXE_CMD_MII_OPMODE_HW, 0, 0, NULL);

	if (err)
		return EIO;
	return 0;
}

static void
axe_uno_mii_statchg(struct ifnet *ifp)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	struct usbnet * const un = ifp->if_softc;
	struct axe_softc * const sc = usbnet_softc(un);
	struct mii_data *mii = usbnet_mii(un);
	int val, err;

	if (usbnet_isdying(un))
		return;

	val = 0;
	if (AXE_IS_172(un)) {
		if (mii->mii_media_active & IFM_FDX)
			val |= AXE_MEDIA_FULL_DUPLEX;
	} else {
		if (mii->mii_media_active & IFM_FDX) {
			val |= AXE_MEDIA_FULL_DUPLEX;
			if (mii->mii_media_active & IFM_ETH_TXPAUSE)
				val |= AXE_178_MEDIA_TXFLOW_CONTROL_EN;
			if (mii->mii_media_active & IFM_ETH_RXPAUSE)
				val |= AXE_178_MEDIA_RXFLOW_CONTROL_EN;
		}
		val |= AXE_178_MEDIA_RX_EN | AXE_178_MEDIA_MAGIC;
		if (un->un_flags & AX178)
			val |= AXE_178_MEDIA_ENCK;
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_1000_T:
			val |= AXE_178_MEDIA_GMII | AXE_178_MEDIA_ENCK;
			usbnet_set_link(un, true);
			break;
		case IFM_100_TX:
			val |= AXE_178_MEDIA_100TX;
			usbnet_set_link(un, true);
			break;
		case IFM_10_T:
			usbnet_set_link(un, true);
			break;
		}
	}

	DPRINTF("val=%#jx", val, 0, 0, 0);
	err = axe_cmd(sc, AXE_CMD_WRITE_MEDIA, 0, val, NULL);
	if (err)
		device_printf(un->un_dev, "media change failed\n");
}

static void
axe_uno_mcast(struct ifnet *ifp)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	struct axe_softc * const sc = usbnet_softc(un);
	struct ethercom *ec = usbnet_ec(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint16_t rxmode;
	uint32_t h = 0;
	uint8_t mchash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (usbnet_isdying(un))
		return;

	if (axe_cmd(sc, AXE_CMD_RXCTL_READ, 0, 0, &rxmode)) {
		device_printf(un->un_dev, "can't read rxmode");
		return;
	}
	rxmode = le16toh(rxmode);

	rxmode &=
	    ~(AXE_RXCMD_ALLMULTI | AXE_RXCMD_PROMISC | AXE_RXCMD_MULTICAST);

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		rxmode |= AXE_RXCMD_ALLMULTI; /* ??? */
		rxmode |= AXE_RXCMD_PROMISC;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			/* accept all mcast frames */
			rxmode |= AXE_RXCMD_ALLMULTI;
			goto update;
		}
		h = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
		mchash[h >> 29] |= 1U << ((h >> 26) & 7);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	if (h != 0)
		rxmode |= AXE_RXCMD_MULTICAST;	/* activate mcast hash filter */
	axe_cmd(sc, AXE_CMD_WRITE_MCAST, 0, 0, mchash);
 update:
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, rxmode, NULL);
}

static void
axe_ax_init(struct usbnet *un)
{
	struct axe_softc * const sc = usbnet_softc(un);

	int cmd = AXE_178_CMD_READ_NODEID;

	if (un->un_flags & AX178) {
		axe_ax88178_init(sc);
	} else if (un->un_flags & AX772) {
		axe_ax88772_init(sc);
	} else if (un->un_flags & AX772A) {
		axe_ax88772a_init(sc);
	} else if (un->un_flags & AX772B) {
		axe_ax88772b_init(sc);
		return;
	} else {
		cmd = AXE_172_CMD_READ_NODEID;
	}

	if (axe_cmd(sc, cmd, 0, 0, un->un_eaddr)) {
		aprint_error_dev(un->un_dev,
		    "failed to read ethernet address\n");
	}
}


static void
axe_reset(struct usbnet *un)
{

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return;

	/*
	 * softnet_lock can be taken when NET_MPAFE is not defined when calling
	 * if_addr_init -> if_init.  This doesn't mix well with the
	 * usbd_delay_ms calls in the init routines as things like nd6_slowtimo
	 * can fire during the wait and attempt to take softnet_lock and then
	 * block the softclk thread meaning the wait never ends.
	 */
#ifndef NET_MPSAFE
	/* XXX What to reset? */

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
#else
	axe_ax_init(un);
#endif
}

static int
axe_get_phyno(struct axe_softc *sc, int sel)
{
	int phyno;

	switch (AXE_PHY_TYPE(sc->axe_phyaddrs[sel])) {
	case PHY_TYPE_100_HOME:
		/* FALLTHROUGH */
	case PHY_TYPE_GIG:
		phyno = AXE_PHY_NO(sc->axe_phyaddrs[sel]);
		break;
	case PHY_TYPE_SPECIAL:
		/* FALLTHROUGH */
	case PHY_TYPE_RSVD:
		/* FALLTHROUGH */
	case PHY_TYPE_NON_SUP:
		/* FALLTHROUGH */
	default:
		phyno = -1;
		break;
	}

	return phyno;
}

#define	AXE_GPIO_WRITE(x, y)	do {				\
	axe_cmd(sc, AXE_CMD_WRITE_GPIO, 0, (x), NULL);		\
	usbd_delay_ms(sc->axe_un.un_udev, hztoms(y));		\
} while (0)

static void
axe_ax88178_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = &sc->axe_un;
	int gpio0, ledmode, phymode;
	uint16_t eeprom, val;

	axe_cmd(sc, AXE_CMD_SROM_WR_ENABLE, 0, 0, NULL);
	/* XXX magic */
	if (axe_cmd(sc, AXE_CMD_SROM_READ, 0, 0x0017, &eeprom) != 0)
		eeprom = 0xffff;
	axe_cmd(sc, AXE_CMD_SROM_WR_DISABLE, 0, 0, NULL);

	eeprom = le16toh(eeprom);

	DPRINTF("EEPROM is %#jx", eeprom, 0, 0, 0);

	/* if EEPROM is invalid we have to use to GPIO0 */
	if (eeprom == 0xffff) {
		phymode = AXE_PHY_MODE_MARVELL;
		gpio0 = 1;
		ledmode = 0;
	} else {
		phymode = eeprom & 0x7f;
		gpio0 = (eeprom & 0x80) ? 0 : 1;
		ledmode = eeprom >> 8;
	}

	DPRINTF("use gpio0: %jd, phymode %jd", gpio0, phymode, 0, 0);

	/* Program GPIOs depending on PHY hardware. */
	switch (phymode) {
	case AXE_PHY_MODE_MARVELL:
		if (gpio0 == 1) {
			AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM | AXE_GPIO0_EN,
			    hz / 32);
			AXE_GPIO_WRITE(AXE_GPIO0_EN | AXE_GPIO2 | AXE_GPIO2_EN,
			    hz / 32);
			AXE_GPIO_WRITE(AXE_GPIO0_EN | AXE_GPIO2_EN, hz / 4);
			AXE_GPIO_WRITE(AXE_GPIO0_EN | AXE_GPIO2 | AXE_GPIO2_EN,
			    hz / 32);
		} else {
			AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM | AXE_GPIO1 |
			    AXE_GPIO1_EN, hz / 3);
			if (ledmode == 1) {
				AXE_GPIO_WRITE(AXE_GPIO1_EN, hz / 3);
				AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN,
				    hz / 3);
			} else {
				AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN |
				    AXE_GPIO2 | AXE_GPIO2_EN, hz / 32);
				AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN |
				    AXE_GPIO2_EN, hz / 4);
				AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN |
				    AXE_GPIO2 | AXE_GPIO2_EN, hz / 32);
			}
		}
		break;
	case AXE_PHY_MODE_CICADA:
	case AXE_PHY_MODE_CICADA_V2:
	case AXE_PHY_MODE_CICADA_V2_ASIX:
		if (gpio0 == 1)
			AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM | AXE_GPIO0 |
			    AXE_GPIO0_EN, hz / 32);
		else
			AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM | AXE_GPIO1 |
			    AXE_GPIO1_EN, hz / 32);
		break;
	case AXE_PHY_MODE_AGERE:
		AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM | AXE_GPIO1 |
		    AXE_GPIO1_EN, hz / 32);
		AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN | AXE_GPIO2 |
		    AXE_GPIO2_EN, hz / 32);
		AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN | AXE_GPIO2_EN, hz / 4);
		AXE_GPIO_WRITE(AXE_GPIO1 | AXE_GPIO1_EN | AXE_GPIO2 |
		    AXE_GPIO2_EN, hz / 32);
		break;
	case AXE_PHY_MODE_REALTEK_8211CL:
	case AXE_PHY_MODE_REALTEK_8211BN:
	case AXE_PHY_MODE_REALTEK_8251CL:
		val = gpio0 == 1 ? AXE_GPIO0 | AXE_GPIO0_EN :
		    AXE_GPIO1 | AXE_GPIO1_EN;
		AXE_GPIO_WRITE(val, hz / 32);
		AXE_GPIO_WRITE(val | AXE_GPIO2 | AXE_GPIO2_EN, hz / 32);
		AXE_GPIO_WRITE(val | AXE_GPIO2_EN, hz / 4);
		AXE_GPIO_WRITE(val | AXE_GPIO2 | AXE_GPIO2_EN, hz / 32);
		if (phymode == AXE_PHY_MODE_REALTEK_8211CL) {
			axe_uno_mii_write_reg(un, un->un_phyno, 0x1F, 0x0005);
			axe_uno_mii_write_reg(un, un->un_phyno, 0x0C, 0x0000);
			axe_uno_mii_read_reg(un, un->un_phyno, 0x0001, &val);
			axe_uno_mii_write_reg(un, un->un_phyno, 0x01, val | 0x0080);
			axe_uno_mii_write_reg(un, un->un_phyno, 0x1F, 0x0000);
		}
		break;
	default:
		/* Unknown PHY model or no need to program GPIOs. */
		break;
	}

	/* soft reset */
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);
	usbd_delay_ms(un->un_udev, 150);
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
	    AXE_SW_RESET_PRL | AXE_178_RESET_MAGIC, NULL);
	usbd_delay_ms(un->un_udev, 150);
	/* Enable MII/GMII/RGMII interface to work with external PHY. */
	axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, 0, NULL);
	usbd_delay_ms(un->un_udev, 10);
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

static void
axe_ax88772_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = &sc->axe_un;

	axe_cmd(sc, AXE_CMD_WRITE_GPIO, 0, 0x00b0, NULL);
	usbd_delay_ms(un->un_udev, 40);

	if (un->un_phyno == AXE_772_PHY_NO_EPHY) {
		/* ask for the embedded PHY */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0,
		    AXE_SW_PHY_SELECT_EMBEDDED, NULL);
		usbd_delay_ms(un->un_udev, 10);

		/* power down and reset state, pin reset state */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);
		usbd_delay_ms(un->un_udev, 60);

		/* power down/reset state, pin operating state */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPPD | AXE_SW_RESET_PRL, NULL);
		usbd_delay_ms(un->un_udev, 150);

		/* power up, reset */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_PRL, NULL);

		/* power up, operating */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPRL | AXE_SW_RESET_PRL, NULL);
	} else {
		/* ask for external PHY */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, AXE_SW_PHY_SELECT_EXT,
		    NULL);
		usbd_delay_ms(un->un_udev, 10);

		/* power down internal PHY */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPPD | AXE_SW_RESET_PRL, NULL);
	}

	usbd_delay_ms(un->un_udev, 150);
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

static void
axe_ax88772_phywake(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = &sc->axe_un;

	if (un->un_phyno == AXE_772_PHY_NO_EPHY) {
		/* Manually select internal(embedded) PHY - MAC mode. */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0,
		    AXE_SW_PHY_SELECT_EMBEDDED, NULL);
		usbd_delay_ms(un->un_udev, hztoms(hz / 32));
	} else {
		/*
		 * Manually select external PHY - MAC mode.
		 * Reverse MII/RMII is for AX88772A PHY mode.
		 */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, AXE_SW_PHY_SELECT_SS_ENB |
		    AXE_SW_PHY_SELECT_EXT | AXE_SW_PHY_SELECT_SS_MII, NULL);
		usbd_delay_ms(un->un_udev, hztoms(hz / 32));
	}

	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPPD |
	    AXE_SW_RESET_IPRL, NULL);

	/* T1 = min 500ns everywhere */
	usbd_delay_ms(un->un_udev, 150);

	/* Take PHY out of power down. */
	if (un->un_phyno == AXE_772_PHY_NO_EPHY) {
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPRL, NULL);
	} else {
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_PRTE, NULL);
	}

	/* 772 T2 is 60ms. 772A T2 is 160ms, 772B T2 is 600ms */
	usbd_delay_ms(un->un_udev, 600);

	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);

	/* T3 = 500ns everywhere */
	usbd_delay_ms(un->un_udev, hztoms(hz / 32));
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPRL, NULL);
	usbd_delay_ms(un->un_udev, hztoms(hz / 32));
}

static void
axe_ax88772a_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	/* Reload EEPROM. */
	AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM, hz / 32);
	axe_ax88772_phywake(sc);
	/* Stop MAC. */
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

static void
axe_ax88772b_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = &sc->axe_un;
	uint16_t eeprom;
	int i;

	/* Reload EEPROM. */
	AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM , hz / 32);

	/*
	 * Save PHY power saving configuration(high byte) and
	 * clear EEPROM checksum value(low byte).
	 */
	if (axe_cmd(sc, AXE_CMD_SROM_READ, 0, AXE_EEPROM_772B_PHY_PWRCFG,
	    &eeprom)) {
		aprint_error_dev(un->un_dev, "failed to read eeprom\n");
		return;
	}

	sc->sc_pwrcfg = le16toh(eeprom) & 0xFF00;

	/*
	 * Auto-loaded default station address from internal ROM is
	 * 00:00:00:00:00:00 such that an explicit access to EEPROM
	 * is required to get real station address.
	 */
	uint8_t *eaddr = un->un_eaddr;
	for (i = 0; i < ETHER_ADDR_LEN / 2; i++) {
		if (axe_cmd(sc, AXE_CMD_SROM_READ, 0,
		    AXE_EEPROM_772B_NODE_ID + i, &eeprom)) {
			aprint_error_dev(un->un_dev,
			    "failed to read eeprom\n");
		    eeprom = 0;
		}
		eeprom = le16toh(eeprom);
		*eaddr++ = (uint8_t)(eeprom & 0xFF);
		*eaddr++ = (uint8_t)((eeprom >> 8) & 0xFF);
	}
	/* Wakeup PHY. */
	axe_ax88772_phywake(sc);
	/* Stop MAC. */
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

#undef	AXE_GPIO_WRITE

/*
 * Probe for a AX88172 chip.
 */
static int
axe_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return axe_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
axe_attach(device_t parent, device_t self, void *aux)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	USBNET_MII_DECL_DEFAULT(unm);
	struct axe_softc *sc = device_private(self);
	struct usbnet * const un = &sc->axe_un;
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	unsigned bufsz;
	int i;

	KASSERT((void *)sc == un);

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &axe_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = AXE_RX_LIST_CNT;
	un->un_tx_list_cnt = AXE_TX_LIST_CNT;

	err = usbd_set_config_no(dev, AXE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	un->un_flags = axe_lookup(uaa->uaa_vendor, uaa->uaa_product)->axe_flags;

	err = usbd_device2interface_handle(dev, AXE_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	id = usbd_get_interface_descriptor(un->un_iface);

	/* decide on what our bufsize will be */
	if (AXE_IS_172(un))
		bufsz = AXE_172_BUFSZ;
	else
		bufsz = (un->un_udev->ud_speed == USB_SPEED_HIGH) ?
		    AXE_178_MAX_BUFSZ : AXE_178_MIN_BUFSZ;
	un->un_rx_bufsz = un->un_tx_bufsz = bufsz;

	un->un_ed[USBNET_ENDPT_RX] = 0;
	un->un_ed[USBNET_ENDPT_TX] = 0;
	un->un_ed[USBNET_ENDPT_INTR] = 0;

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return;
		}
		const uint8_t xt = UE_GET_XFERTYPE(ed->bmAttributes);
		const uint8_t dir = UE_GET_DIR(ed->bEndpointAddress);

		if (dir == UE_DIR_IN && xt == UE_BULK &&
		    un->un_ed[USBNET_ENDPT_RX] == 0) {
			un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
		} else if (dir == UE_DIR_OUT && xt == UE_BULK &&
		    un->un_ed[USBNET_ENDPT_TX] == 0) {
			un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
		} else if (dir == UE_DIR_IN && xt == UE_INTERRUPT) {
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	/* Set these up now for axe_cmd().  */
	usbnet_attach(un, "axedet");

	/* We need the PHYID for init dance in some cases */
	usbnet_lock_core(un);
	if (axe_cmd(sc, AXE_CMD_READ_PHYID, 0, 0, &sc->axe_phyaddrs)) {
		aprint_error_dev(self, "failed to read phyaddrs\n");
		usbnet_unlock_core(un);
		return;
	}

	DPRINTF(" phyaddrs[0]: %jx phyaddrs[1]: %jx",
	    sc->axe_phyaddrs[0], sc->axe_phyaddrs[1], 0, 0);
	un->un_phyno = axe_get_phyno(sc, AXE_PHY_SEL_PRI);
	if (un->un_phyno == -1)
		un->un_phyno = axe_get_phyno(sc, AXE_PHY_SEL_SEC);
	if (un->un_phyno == -1) {
		DPRINTF(" no valid PHY address found, assuming PHY address 0",
		    0, 0, 0, 0);
		un->un_phyno = 0;
	}

	/* Initialize controller and get station address. */

	axe_ax_init(un);

	/*
	 * Fetch IPG values.
	 */
	if (un->un_flags & (AX772A | AX772B)) {
		/* Set IPG values. */
		sc->axe_ipgs[0] = AXE_IPG0_DEFAULT;
		sc->axe_ipgs[1] = AXE_IPG1_DEFAULT;
		sc->axe_ipgs[2] = AXE_IPG2_DEFAULT;
	} else {
		if (axe_cmd(sc, AXE_CMD_READ_IPG012, 0, 0, sc->axe_ipgs)) {
			aprint_error_dev(self, "failed to read ipg\n");
			usbnet_unlock_core(un);
			return;
		}
	}

	usbnet_unlock_core(un);

	if (!AXE_IS_172(un))
		usbnet_ec(un)->ec_capabilities = ETHERCAP_VLAN_MTU;
	if (un->un_flags & AX772B) {
		struct ifnet *ifp = usbnet_ifp(un);

		ifp->if_capabilities =
		    IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
		    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;
		/*
		 * Checksum offloading of AX88772B also works with VLAN
		 * tagged frames but there is no way to take advantage
		 * of the feature because vlan(4) assumes
		 * IFCAP_VLAN_HWTAGGING is prerequisite condition to
		 * support checksum offloading with VLAN. VLAN hardware
		 * tagging support of AX88772B is very limited so it's
		 * not possible to announce IFCAP_VLAN_HWTAGGING.
		 */
	}
	if (un->un_flags & (AX772A | AX772B | AX178))
		unm.un_mii_flags = MIIF_DOPAUSE;

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static void
axe_uno_rx_loop(struct usbnet * un, struct usbnet_chain *c, uint32_t total_len)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc * const sc = usbnet_softc(un);
	struct ifnet *ifp = usbnet_ifp(un);
	uint8_t *buf = c->unc_buf;

	do {
		u_int pktlen = 0;
		u_int rxlen = 0;
		int flags = 0;

		if ((un->un_flags & AXSTD_FRAME) != 0) {
			struct axe_sframe_hdr hdr;

			if (total_len < sizeof(hdr)) {
				if_statinc(ifp, if_ierrors);
				break;
			}

			memcpy(&hdr, buf, sizeof(hdr));

			DPRINTFN(20, "total_len %#jx len %#jx ilen %#jx",
			    total_len,
			    (le16toh(hdr.len) & AXE_RH1M_RXLEN_MASK),
			    (le16toh(hdr.ilen) & AXE_RH1M_RXLEN_MASK), 0);

			total_len -= sizeof(hdr);
			buf += sizeof(hdr);

			if (((le16toh(hdr.len) & AXE_RH1M_RXLEN_MASK) ^
			    (le16toh(hdr.ilen) & AXE_RH1M_RXLEN_MASK)) !=
			    AXE_RH1M_RXLEN_MASK) {
				if_statinc(ifp, if_ierrors);
				break;
			}

			rxlen = le16toh(hdr.len) & AXE_RH1M_RXLEN_MASK;
			if (total_len < rxlen) {
				pktlen = total_len;
				total_len = 0;
			} else {
				pktlen = rxlen;
				rxlen = roundup2(rxlen, 2);
				total_len -= rxlen;
			}

		} else if ((un->un_flags & AXCSUM_FRAME) != 0) {
			struct axe_csum_hdr csum_hdr;

			if (total_len <	sizeof(csum_hdr)) {
				if_statinc(ifp, if_ierrors);
				break;
			}

			memcpy(&csum_hdr, buf, sizeof(csum_hdr));

			csum_hdr.len = le16toh(csum_hdr.len);
			csum_hdr.ilen = le16toh(csum_hdr.ilen);
			csum_hdr.cstatus = le16toh(csum_hdr.cstatus);

			DPRINTFN(20, "total_len %#jx len %#jx ilen %#jx"
			    " cstatus %#jx", total_len,
			    csum_hdr.len, csum_hdr.ilen, csum_hdr.cstatus);

			if ((AXE_CSUM_RXBYTES(csum_hdr.len) ^
			    AXE_CSUM_RXBYTES(csum_hdr.ilen)) !=
			    sc->sc_lenmask) {
				/* we lost sync */
				if_statinc(ifp, if_ierrors);
				DPRINTFN(20, "len %#jx ilen %#jx lenmask %#jx "
				    "err",
				    AXE_CSUM_RXBYTES(csum_hdr.len),
				    AXE_CSUM_RXBYTES(csum_hdr.ilen),
				    sc->sc_lenmask, 0);
				break;
			}
			/*
			 * Get total transferred frame length including
			 * checksum header.  The length should be multiple
			 * of 4.
			 */
			pktlen = AXE_CSUM_RXBYTES(csum_hdr.len);
			u_int len = sizeof(csum_hdr) + pktlen;
			len = (len + 3) & ~3;
			if (total_len < len) {
				DPRINTFN(20, "total_len %#jx < len %#jx",
				    total_len, len, 0, 0);
				/* invalid length */
				if_statinc(ifp, if_ierrors);
				break;
			}
			buf += sizeof(csum_hdr);

			const uint16_t cstatus = csum_hdr.cstatus;

			if (cstatus & AXE_CSUM_HDR_L3_TYPE_IPV4) {
				if (cstatus & AXE_CSUM_HDR_L4_CSUM_ERR)
					flags |= M_CSUM_TCP_UDP_BAD;
				if (cstatus & AXE_CSUM_HDR_L3_CSUM_ERR)
					flags |= M_CSUM_IPv4_BAD;

				const uint16_t l4type =
				    cstatus & AXE_CSUM_HDR_L4_TYPE_MASK;

				if (l4type == AXE_CSUM_HDR_L4_TYPE_TCP)
					flags |= M_CSUM_TCPv4;
				if (l4type == AXE_CSUM_HDR_L4_TYPE_UDP)
					flags |= M_CSUM_UDPv4;
			}
			if (total_len < len) {
				pktlen = total_len;
				total_len = 0;
			} else {
				total_len -= len;
				rxlen = len - sizeof(csum_hdr);
			}
			DPRINTFN(20, "total_len %#jx len %#jx pktlen %#jx"
			    " rxlen %#jx", total_len, len, pktlen, rxlen);
		} else { /* AX172 */
			pktlen = rxlen = total_len;
			total_len = 0;
		}

		usbnet_enqueue(un, buf, pktlen, flags, 0, 0);
		buf += rxlen;

	} while (total_len > 0);

	DPRINTFN(10, "start rx", 0, 0, 0, 0);
}

static unsigned
axe_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_sframe_hdr hdr, tlr;
	size_t hdr_len = 0, tlr_len = 0;
	int length, boundary;

	if (!AXE_IS_172(un)) {
		/*
		 * Copy the mbuf data into a contiguous buffer, leaving two
		 * bytes at the beginning to hold the frame length.
		 */
		boundary = (un->un_udev->ud_speed == USB_SPEED_HIGH) ? 512 : 64;

		hdr.len = htole16(m->m_pkthdr.len);
		hdr.ilen = ~hdr.len;
		hdr_len = sizeof(hdr);

		length = hdr_len + m->m_pkthdr.len;

		if ((length % boundary) == 0) {
			tlr.len = 0x0000;
			tlr.ilen = 0xffff;
			tlr_len = sizeof(tlr);
		}
		DPRINTFN(20, "length %jx m_pkthdr.len %jx hdrsize %#jx",
			length, m->m_pkthdr.len, sizeof(hdr), 0);
	}

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - hdr_len - tlr_len)
		return 0;
	length = hdr_len + m->m_pkthdr.len + tlr_len;

	if (hdr_len)
		memcpy(c->unc_buf, &hdr, hdr_len);
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf + hdr_len);
	if (tlr_len)
		memcpy(c->unc_buf + length - tlr_len, &tlr, tlr_len);

	return length;
}

static void
axe_csum_cfg(struct axe_softc *sc)
{
	struct usbnet * const un = &sc->axe_un;
	struct ifnet * const ifp = usbnet_ifp(un);
	uint16_t csum1, csum2;

	if ((un->un_flags & AX772B) != 0) {
		csum1 = 0;
		csum2 = 0;
		if ((ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) != 0)
			csum1 |= AXE_TXCSUM_IP;
		if ((ifp->if_capenable & IFCAP_CSUM_TCPv4_Tx) != 0)
			csum1 |= AXE_TXCSUM_TCP;
		if ((ifp->if_capenable & IFCAP_CSUM_UDPv4_Tx) != 0)
			csum1 |= AXE_TXCSUM_UDP;
		if ((ifp->if_capenable & IFCAP_CSUM_TCPv6_Tx) != 0)
			csum1 |= AXE_TXCSUM_TCPV6;
		if ((ifp->if_capenable & IFCAP_CSUM_UDPv6_Tx) != 0)
			csum1 |= AXE_TXCSUM_UDPV6;
		axe_cmd(sc, AXE_772B_CMD_WRITE_TXCSUM, csum2, csum1, NULL);
		csum1 = 0;
		csum2 = 0;

		if ((ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) != 0)
			csum1 |= AXE_RXCSUM_IP;
		if ((ifp->if_capenable & IFCAP_CSUM_TCPv4_Rx) != 0)
			csum1 |= AXE_RXCSUM_TCP;
		if ((ifp->if_capenable & IFCAP_CSUM_UDPv4_Rx) != 0)
			csum1 |= AXE_RXCSUM_UDP;
		if ((ifp->if_capenable & IFCAP_CSUM_TCPv6_Rx) != 0)
			csum1 |= AXE_RXCSUM_TCPV6;
		if ((ifp->if_capenable & IFCAP_CSUM_UDPv6_Rx) != 0)
			csum1 |= AXE_RXCSUM_UDPV6;
		axe_cmd(sc, AXE_772B_CMD_WRITE_RXCSUM, csum2, csum1, NULL);
	}
}

static int
axe_uno_init(struct ifnet *ifp)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	struct axe_softc * const sc = usbnet_softc(un);
	int rxmode;

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O */
	usbnet_stop(un, ifp, 1);

	/* Reset the ethernet interface. */
	axe_reset(un);

#if 0
	ret = asix_write_gpio(dev, AX_GPIO_RSE | AX_GPIO_GPO_2 |
			      AX_GPIO_GPO2EN, 5, in_pm);
#endif
	/* Set MAC address and transmitter IPG values. */
	if (AXE_IS_172(un)) {
		axe_cmd(sc, AXE_172_CMD_WRITE_NODEID, 0, 0, un->un_eaddr);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG0, 0, sc->axe_ipgs[0], NULL);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG1, 0, sc->axe_ipgs[1], NULL);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG2, 0, sc->axe_ipgs[2], NULL);
	} else {
		axe_cmd(sc, AXE_178_CMD_WRITE_NODEID, 0, 0, un->un_eaddr);
		axe_cmd(sc, AXE_178_CMD_WRITE_IPG012, sc->axe_ipgs[2],
		    (sc->axe_ipgs[1] << 8) | (sc->axe_ipgs[0]), NULL);

		un->un_flags &= ~(AXSTD_FRAME | AXCSUM_FRAME);
		if ((un->un_flags & AX772B) != 0 &&
		    (ifp->if_capenable & AX_RXCSUM) != 0) {
			sc->sc_lenmask = AXE_CSUM_HDR_LEN_MASK;
			un->un_flags |= AXCSUM_FRAME;
		} else {
			sc->sc_lenmask = AXE_HDR_LEN_MASK;
			un->un_flags |= AXSTD_FRAME;
		}
	}

	/* Configure TX/RX checksum offloading. */
	axe_csum_cfg(sc);

	if (un->un_flags & AX772B) {
		/* AX88772B uses different maximum frame burst configuration. */
		axe_cmd(sc, AXE_772B_CMD_RXCTL_WRITE_CFG,
		    ax88772b_mfb_table[AX88772B_MFB_16K].threshold,
		    ax88772b_mfb_table[AX88772B_MFB_16K].byte_cnt, NULL);
	}
	/* Enable receiver, set RX mode */
	rxmode = (AXE_RXCMD_BROADCAST | AXE_RXCMD_MULTICAST | AXE_RXCMD_ENABLE);
	if (AXE_IS_172(un))
		rxmode |= AXE_172_RXCMD_UNICAST;
	else {
		if (un->un_flags & AX772B) {
			/*
			 * Select RX header format type 1.  Aligning IP
			 * header on 4 byte boundary is not needed when
			 * checksum offloading feature is not used
			 * because we always copy the received frame in
			 * RX handler.  When RX checksum offloading is
			 * active, aligning IP header is required to
			 * reflect actual frame length including RX
			 * header size.
			 */
			rxmode |= AXE_772B_RXCMD_HDR_TYPE_1;
			if (un->un_flags & AXCSUM_FRAME)
				rxmode |= AXE_772B_RXCMD_IPHDR_ALIGN;
		} else {
			/*
			 * Default Rx buffer size is too small to get
			 * maximum performance.
			 */
#if 0
			if (un->un_udev->ud_speed == USB_SPEED_HIGH) {
				/* Largest possible USB buffer size for AX88178 */
			}
#endif
			rxmode |= AXE_178_RXCMD_MFB_16384;
		}
	}

	DPRINTF("rxmode %#jx", rxmode, 0, 0, 0);

	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, rxmode, NULL);

	/* Accept multicast frame or run promisc. mode */
	axe_uno_mcast(ifp);

	return usbnet_init_rx_tx(un);
}

static void
axe_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	axe_reset(un);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(axe)
