/*	$NetBSD: if_axe.c,v 1.85 2018/04/20 11:14:40 christos Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: if_axe.c,v 1.85 2018/04/20 11:14:40 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_axereg.h>

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
static int axedebug = 20;

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
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
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
	{ { USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_UFE2000}, 0 },
	{ { USB_VENDOR_ACERCM,		USB_PRODUCT_ACERCM_EP1427X2}, 0 },
	{ { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_ETHERNET }, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88172}, 0 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772}, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772A}, AX772 },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772B}, AX772B },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88772B_1}, AX772B },
	{ { USB_VENDOR_ASIX,		USB_PRODUCT_ASIX_AX88178}, AX178 },
	{ { USB_VENDOR_ATEN,		USB_PRODUCT_ATEN_UC210T}, 0 },
	{ { USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_F5D5055 }, AX178 },
	{ { USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USB2AR}, 0},
	{ { USB_VENDOR_CISCOLINKSYS,	USB_PRODUCT_CISCOLINKSYS_USB200MV2}, AX772A },
	{ { USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB2_TX }, 0},
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100}, 0 },
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100B1 }, AX772 },
	{ { USB_VENDOR_DLINK2,		USB_PRODUCT_DLINK2_DUBE100B1 }, AX772 },
	{ { USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100C1 }, AX772B },
	{ { USB_VENDOR_GOODWAY,		USB_PRODUCT_GOODWAY_GWUSB2E}, 0 },
	{ { USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_ETGUS2 }, AX178 },
	{ { USB_VENDOR_JVC,		USB_PRODUCT_JVC_MP_PRX1}, 0 },
	{ { USB_VENDOR_LENOVO,		USB_PRODUCT_LENOVO_ETHERNET }, AX772B },
	{ { USB_VENDOR_LINKSYS, 	USB_PRODUCT_LINKSYS_HG20F9}, AX772B },
	{ { USB_VENDOR_LINKSYS2,	USB_PRODUCT_LINKSYS2_USB200M}, 0 },
	{ { USB_VENDOR_LINKSYS4,	USB_PRODUCT_LINKSYS4_USB1000 }, AX178 },
	{ { USB_VENDOR_LOGITEC,		USB_PRODUCT_LOGITEC_LAN_GTJU2}, AX178 },
	{ { USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUAU2GT}, AX178 },
	{ { USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUAU2KTX}, 0 },
	{ { USB_VENDOR_MSI,		USB_PRODUCT_MSI_AX88772A}, AX772 },
	{ { USB_VENDOR_NETGEAR,		USB_PRODUCT_NETGEAR_FA120}, 0 },
	{ { USB_VENDOR_OQO,		USB_PRODUCT_OQO_ETHER01PLUS }, AX772 },
	{ { USB_VENDOR_PLANEX3,		USB_PRODUCT_PLANEX3_GU1000T }, AX178 },
	{ { USB_VENDOR_SITECOM,		USB_PRODUCT_SITECOM_LN029}, 0 },
	{ { USB_VENDOR_SITECOMEU,	USB_PRODUCT_SITECOMEU_LN028 }, AX178 },
	{ { USB_VENDOR_SITECOMEU,	USB_PRODUCT_SITECOMEU_LN031 }, AX178 },
	{ { USB_VENDOR_SYSTEMTALKS,	USB_PRODUCT_SYSTEMTALKS_SGCX2UL}, 0 },
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

int	axe_match(device_t, cfdata_t, void *);
void	axe_attach(device_t, device_t, void *);
int	axe_detach(device_t, int);
int	axe_activate(device_t, devact_t);

CFATTACH_DECL_NEW(axe, sizeof(struct axe_softc),
	axe_match, axe_attach, axe_detach, axe_activate);

static int	axe_tx_list_init(struct axe_softc *);
static int	axe_rx_list_init(struct axe_softc *);
static int	axe_encap(struct axe_softc *, struct mbuf *, int);
static void	axe_rxeof(struct usbd_xfer *, void *, usbd_status);
static void	axe_txeof(struct usbd_xfer *, void *, usbd_status);
static void	axe_tick(void *);
static void	axe_tick_task(void *);
static void	axe_start(struct ifnet *);
static int	axe_ioctl(struct ifnet *, u_long, void *);
static int	axe_init(struct ifnet *);
static void	axe_stop(struct ifnet *, int);
static void	axe_watchdog(struct ifnet *);
static int	axe_miibus_readreg_locked(device_t, int, int);
static int	axe_miibus_readreg(device_t, int, int);
static void	axe_miibus_writereg_locked(device_t, int, int, int);
static void	axe_miibus_writereg(device_t, int, int, int);
static void	axe_miibus_statchg(struct ifnet *);
static int	axe_cmd(struct axe_softc *, int, int, int, void *);
static void	axe_reset(struct axe_softc *);

static void	axe_setmulti(struct axe_softc *);
static void	axe_lock_mii(struct axe_softc *);
static void	axe_unlock_mii(struct axe_softc *);

static void	axe_ax88178_init(struct axe_softc *);
static void	axe_ax88772_init(struct axe_softc *);
static void	axe_ax88772a_init(struct axe_softc *);
static void	axe_ax88772b_init(struct axe_softc *);

/* Get exclusive access to the MII registers */
static void
axe_lock_mii(struct axe_softc *sc)
{

	sc->axe_refcnt++;
	mutex_enter(&sc->axe_mii_lock);
}

static void
axe_unlock_mii(struct axe_softc *sc)
{

	mutex_exit(&sc->axe_mii_lock);
	if (--sc->axe_refcnt < 0)
		usb_detach_wakeupold((sc->axe_dev));
}

static int
axe_cmd(struct axe_softc *sc, int cmd, int index, int val, void *buf)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	usb_device_request_t req;
	usbd_status err;

	KASSERT(mutex_owned(&sc->axe_mii_lock));

	if (sc->axe_dying)
		return 0;

	DPRINTFN(20, "cmd %#jx index %#jx val %#jx", cmd, index, val, 0);

	if (AXE_CMD_DIR(cmd))
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AXE_CMD_CMD(cmd);
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, AXE_CMD_LEN(cmd));

	err = usbd_do_request(sc->axe_udev, &req, buf);

	if (err) {
		DPRINTF("cmd %jd err %jd", cmd, err, 0, 0);
		return -1;
	}
	return 0;
}

static int
axe_miibus_readreg_locked(device_t dev, int phy, int reg)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = device_private(dev);
	usbd_status err;
	uint16_t val;

	DPRINTFN(30, "phy 0x%jx reg 0x%jx\n", phy, reg, 0, 0);

	axe_cmd(sc, AXE_CMD_MII_OPMODE_SW, 0, 0, NULL);

	err = axe_cmd(sc, AXE_CMD_MII_READ_REG, reg, phy, (void *)&val);
	axe_cmd(sc, AXE_CMD_MII_OPMODE_HW, 0, 0, NULL);
	if (err) {
		aprint_error_dev(sc->axe_dev, "read PHY failed\n");
		return -1;
	}

	val = le16toh(val);
	if (AXE_IS_772(sc) && reg == MII_BMSR) {
		/*
		 * BMSR of AX88772 indicates that it supports extended
		 * capability but the extended status register is
		 * reserved for embedded ethernet PHY. So clear the
		 * extended capability bit of BMSR.
		 */
		 val &= ~BMSR_EXTCAP;
	}

	DPRINTFN(30, "phy 0x%jx reg 0x%jx val %#jx", phy, reg, val, 0);

	return val;
}

static int
axe_miibus_readreg(device_t dev, int phy, int reg)
{
	struct axe_softc *sc = device_private(dev);
	int val;

	if (sc->axe_dying)
		return 0;

	if (sc->axe_phyno != phy)
		return 0;

	axe_lock_mii(sc);
	val = axe_miibus_readreg_locked(dev, phy, reg);
	axe_unlock_mii(sc);

	return val;
}

static void
axe_miibus_writereg_locked(device_t dev, int phy, int reg, int aval)
{
	struct axe_softc *sc = device_private(dev);
	usbd_status err;
	uint16_t val;

	val = htole16(aval);

	axe_cmd(sc, AXE_CMD_MII_OPMODE_SW, 0, 0, NULL);
	err = axe_cmd(sc, AXE_CMD_MII_WRITE_REG, reg, phy, (void *)&val);
	axe_cmd(sc, AXE_CMD_MII_OPMODE_HW, 0, 0, NULL);

	if (err) {
		aprint_error_dev(sc->axe_dev, "write PHY failed\n");
		return;
	}
}

static void
axe_miibus_writereg(device_t dev, int phy, int reg, int aval)
{
	struct axe_softc *sc = device_private(dev);

	if (sc->axe_dying)
		return;

	if (sc->axe_phyno != phy)
		return;

	axe_lock_mii(sc);
	axe_miibus_writereg_locked(dev, phy, reg, aval);
	axe_unlock_mii(sc);
}

static void
axe_miibus_statchg(struct ifnet *ifp)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	struct axe_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->axe_mii;
	int val, err;

	val = 0;
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0) {
		val |= AXE_MEDIA_FULL_DUPLEX;
		if (AXE_IS_178_FAMILY(sc)) {
			if ((IFM_OPTIONS(mii->mii_media_active) &
			    IFM_ETH_TXPAUSE) != 0)
				val |= AXE_178_MEDIA_TXFLOW_CONTROL_EN;
			if ((IFM_OPTIONS(mii->mii_media_active) &
			    IFM_ETH_RXPAUSE) != 0)
				val |= AXE_178_MEDIA_RXFLOW_CONTROL_EN;
		}
	}
	if (AXE_IS_178_FAMILY(sc)) {
		val |= AXE_178_MEDIA_RX_EN | AXE_178_MEDIA_MAGIC;
		if (sc->axe_flags & AX178)
			val |= AXE_178_MEDIA_ENCK;
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_1000_T:
			val |= AXE_178_MEDIA_GMII | AXE_178_MEDIA_ENCK;
			break;
		case IFM_100_TX:
			val |= AXE_178_MEDIA_100TX;
			break;
		case IFM_10_T:
			/* doesn't need to be handled */
			break;
		}
	}

	DPRINTF("val=0x%jx", val, 0, 0, 0);
	axe_lock_mii(sc);
	err = axe_cmd(sc, AXE_CMD_WRITE_MEDIA, 0, val, NULL);
	axe_unlock_mii(sc);
	if (err) {
		aprint_error_dev(sc->axe_dev, "media change failed\n");
		return;
	}
}

static void
axe_setmulti(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct ifnet *ifp = &sc->sc_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t h = 0;
	uint16_t rxmode;
	uint8_t hashtbl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (sc->axe_dying)
		return;

	axe_lock_mii(sc);
	axe_cmd(sc, AXE_CMD_RXCTL_READ, 0, 0, (void *)&rxmode);
	rxmode = le16toh(rxmode);

	rxmode &=
	    ~(AXE_RXCMD_ALLMULTI | AXE_RXCMD_PROMISC |
	    AXE_RXCMD_BROADCAST | AXE_RXCMD_MULTICAST);

	rxmode |=
	    (ifp->if_flags & IFF_BROADCAST) ? AXE_RXCMD_BROADCAST : 0;

	if (ifp->if_flags & (IFF_ALLMULTI | IFF_PROMISC)) {
		if (ifp->if_flags & IFF_PROMISC)
			rxmode |= AXE_RXCMD_PROMISC;
		goto allmulti;
	}

	/* Now program new ones */
	ETHER_FIRST_MULTI(step, &sc->axe_ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0)
			goto allmulti;

		h = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN) >> 26;
		hashtbl[h >> 3] |= 1U << (h & 7);
		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	rxmode |= AXE_RXCMD_MULTICAST;

	axe_cmd(sc, AXE_CMD_WRITE_MCAST, 0, 0, (void *)&hashtbl);
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, rxmode, NULL);
	axe_unlock_mii(sc);
	return;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	rxmode |= AXE_RXCMD_ALLMULTI;
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, rxmode, NULL);
	axe_unlock_mii(sc);
}


static void
axe_reset(struct axe_softc *sc)
{

	if (sc->axe_dying)
		return;

	/*
	 * softnet_lock can be taken when NET_MPAFE is not defined when calling
	 * if_addr_init -> if_init.  This doesn't mixe well with the
	 * usbd_delay_ms calls in the init routines as things like nd6_slowtimo
	 * can fire during the wait and attempt to take softnet_lock and then
	 * block the softclk thread meaing the wait never ends.
	 */
#ifndef NET_MPSAFE
	/* XXX What to reset? */

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
#else
	axe_lock_mii(sc);

	if (sc->axe_flags & AX178) {
		axe_ax88178_init(sc);
	} else if (sc->axe_flags & AX772) {
		axe_ax88772_init(sc);
	} else if (sc->axe_flags & AX772A) {
		axe_ax88772a_init(sc);
	} else if (sc->axe_flags & AX772B) {
		axe_ax88772b_init(sc);
	}
	axe_unlock_mii(sc);
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
	usbd_delay_ms(sc->axe_udev, hztoms(y));			\
} while (0)

static void
axe_ax88178_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	int gpio0, ledmode, phymode;
	uint16_t eeprom, val;

	eeprom = 0;	/* XXX gcc -Wuninitialized */

	axe_cmd(sc, AXE_CMD_SROM_WR_ENABLE, 0, 0, NULL);
	/* XXX magic */
	axe_cmd(sc, AXE_CMD_SROM_READ, 0, 0x0017, &eeprom);
	axe_cmd(sc, AXE_CMD_SROM_WR_DISABLE, 0, 0, NULL);

	eeprom = le16toh(eeprom);

	DPRINTF("EEPROM is 0x%jx", eeprom, 0, 0, 0);

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
			axe_miibus_writereg_locked(sc->axe_dev,
			    sc->axe_phyno, 0x1F, 0x0005);
			axe_miibus_writereg_locked(sc->axe_dev,
			    sc->axe_phyno, 0x0C, 0x0000);
			val = axe_miibus_readreg_locked(sc->axe_dev,
			    sc->axe_phyno, 0x0001);
			axe_miibus_writereg_locked(sc->axe_dev,
			    sc->axe_phyno, 0x01, val | 0x0080);
			axe_miibus_writereg_locked(sc->axe_dev,
			    sc->axe_phyno, 0x1F, 0x0000);
		}
		break;
	default:
		/* Unknown PHY model or no need to program GPIOs. */
		break;
	}

	/* soft reset */
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);
	usbd_delay_ms(sc->axe_udev, 150);
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
	    AXE_SW_RESET_PRL | AXE_178_RESET_MAGIC, NULL);
	usbd_delay_ms(sc->axe_udev, 150);
	/* Enable MII/GMII/RGMII interface to work with external PHY. */
	axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, 0, NULL);
	usbd_delay_ms(sc->axe_udev, 10);
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

static void
axe_ax88772_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	axe_cmd(sc, AXE_CMD_WRITE_GPIO, 0, 0x00b0, NULL);
	usbd_delay_ms(sc->axe_udev, 40);

	if (sc->axe_phyno == AXE_772_PHY_NO_EPHY) {
		/* ask for the embedded PHY */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0,
		    AXE_SW_PHY_SELECT_EMBEDDED, NULL);
		usbd_delay_ms(sc->axe_udev, 10);

		/* power down and reset state, pin reset state */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);
		usbd_delay_ms(sc->axe_udev, 60);

		/* power down/reset state, pin operating state */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPPD | AXE_SW_RESET_PRL, NULL);
		usbd_delay_ms(sc->axe_udev, 150);

		/* power up, reset */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_PRL, NULL);

		/* power up, operating */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPRL | AXE_SW_RESET_PRL, NULL);
	} else {
		/* ask for external PHY */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, AXE_SW_PHY_SELECT_EXT,
		    NULL);
		usbd_delay_ms(sc->axe_udev, 10);

		/* power down internal PHY */
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0,
		    AXE_SW_RESET_IPPD | AXE_SW_RESET_PRL, NULL);
	}

	usbd_delay_ms(sc->axe_udev, 150);
	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, 0, NULL);
}

static void
axe_ax88772_phywake(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	if (sc->axe_phyno == AXE_772_PHY_NO_EPHY) {
		/* Manually select internal(embedded) PHY - MAC mode. */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0,
		    AXE_SW_PHY_SELECT_EMBEDDED,
		    NULL);
		usbd_delay_ms(sc->axe_udev, hztoms(hz / 32));
	} else {
		/*
		 * Manually select external PHY - MAC mode.
		 * Reverse MII/RMII is for AX88772A PHY mode.
		 */
		axe_cmd(sc, AXE_CMD_SW_PHY_SELECT, 0, AXE_SW_PHY_SELECT_SS_ENB |
		    AXE_SW_PHY_SELECT_EXT | AXE_SW_PHY_SELECT_SS_MII, NULL);
		usbd_delay_ms(sc->axe_udev, hztoms(hz / 32));
	}

	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPPD |
	    AXE_SW_RESET_IPRL, NULL);

	/* T1 = min 500ns everywhere */
	usbd_delay_ms(sc->axe_udev, 150);

	/* Take PHY out of power down. */
	if (sc->axe_phyno == AXE_772_PHY_NO_EPHY) {
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPRL, NULL);
	} else {
		axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_PRTE, NULL);
	}

	/* 772 T2 is 60ms. 772A T2 is 160ms, 772B T2 is 600ms */
	usbd_delay_ms(sc->axe_udev, 600);

	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_CLEAR, NULL);

	/* T3 = 500ns everywhere */
	usbd_delay_ms(sc->axe_udev, hztoms(hz / 32));
	axe_cmd(sc, AXE_CMD_SW_RESET_REG, 0, AXE_SW_RESET_IPRL, NULL);
	usbd_delay_ms(sc->axe_udev, hztoms(hz / 32));
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
	uint16_t eeprom;
	int i;

	/* Reload EEPROM. */
	AXE_GPIO_WRITE(AXE_GPIO_RELOAD_EEPROM , hz / 32);

	/*
	 * Save PHY power saving configuration(high byte) and
	 * clear EEPROM checksum value(low byte).
	 */
	axe_cmd(sc, AXE_CMD_SROM_READ, 0, AXE_EEPROM_772B_PHY_PWRCFG, &eeprom);
	sc->sc_pwrcfg = le16toh(eeprom) & 0xFF00;

	/*
	 * Auto-loaded default station address from internal ROM is
	 * 00:00:00:00:00:00 such that an explicit access to EEPROM
	 * is required to get real station address.
	 */
	uint8_t *eaddr = sc->axe_enaddr;
	for (i = 0; i < ETHER_ADDR_LEN / 2; i++) {
		axe_cmd(sc, AXE_CMD_SROM_READ, 0, AXE_EEPROM_772B_NODE_ID + i,
		    &eeprom);
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
int
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
void
axe_attach(device_t parent, device_t self, void *aux)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct mii_data	*mii;
	char *devinfop;
	const char *devname = device_xname(self);
	struct ifnet *ifp;
	int i, s;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->axe_dev = self;
	sc->axe_udev = dev;

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, AXE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	sc->axe_flags = axe_lookup(uaa->uaa_vendor, uaa->uaa_product)->axe_flags;

	mutex_init(&sc->axe_mii_lock, MUTEX_DEFAULT, IPL_NONE);
	usb_init_task(&sc->axe_tick_task, axe_tick_task, sc, 0);

	err = usbd_device2interface_handle(dev, AXE_IFACE_IDX, &sc->axe_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	sc->axe_product = uaa->uaa_product;
	sc->axe_vendor = uaa->uaa_vendor;

	id = usbd_get_interface_descriptor(sc->axe_iface);

	/* decide on what our bufsize will be */
	if (AXE_IS_178_FAMILY(sc))
		sc->axe_bufsz = (sc->axe_udev->ud_speed == USB_SPEED_HIGH) ?
		    AXE_178_MAX_BUFSZ : AXE_178_MIN_BUFSZ;
	else
		sc->axe_bufsz = AXE_172_BUFSZ;

	sc->axe_ed[AXE_ENDPT_RX] = -1;
	sc->axe_ed[AXE_ENDPT_TX] = -1;
	sc->axe_ed[AXE_ENDPT_INTR] = -1;

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->axe_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return;
		}
		const uint8_t xt = UE_GET_XFERTYPE(ed->bmAttributes);
		const uint8_t dir = UE_GET_DIR(ed->bEndpointAddress);

		if (dir == UE_DIR_IN && xt == UE_BULK &&
		    sc->axe_ed[AXE_ENDPT_RX] == -1) {
			sc->axe_ed[AXE_ENDPT_RX] = ed->bEndpointAddress;
		} else if (dir == UE_DIR_OUT && xt == UE_BULK &&
		    sc->axe_ed[AXE_ENDPT_TX] == -1) {
			sc->axe_ed[AXE_ENDPT_TX] = ed->bEndpointAddress;
		} else if (dir == UE_DIR_IN && xt == UE_INTERRUPT) {
			sc->axe_ed[AXE_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	s = splnet();

	/* We need the PHYID for init dance in some cases */
	axe_lock_mii(sc);
	axe_cmd(sc, AXE_CMD_READ_PHYID, 0, 0, (void *)&sc->axe_phyaddrs);

	DPRINTF(" phyaddrs[0]: %jx phyaddrs[1]: %jx",
	    sc->axe_phyaddrs[0], sc->axe_phyaddrs[1], 0, 0);
	sc->axe_phyno = axe_get_phyno(sc, AXE_PHY_SEL_PRI);
	if (sc->axe_phyno == -1)
		sc->axe_phyno = axe_get_phyno(sc, AXE_PHY_SEL_SEC);
	if (sc->axe_phyno == -1) {
		DPRINTF(" no valid PHY address found, assuming PHY address 0",
		    0, 0, 0, 0);
		sc->axe_phyno = 0;
	}

	/* Initialize controller and get station address. */

	if (sc->axe_flags & AX178) {
		axe_ax88178_init(sc);
		axe_cmd(sc, AXE_178_CMD_READ_NODEID, 0, 0, sc->axe_enaddr);
	} else if (sc->axe_flags & AX772) {
		axe_ax88772_init(sc);
		axe_cmd(sc, AXE_178_CMD_READ_NODEID, 0, 0, sc->axe_enaddr);
	} else if (sc->axe_flags & AX772A) {
		axe_ax88772a_init(sc);
		axe_cmd(sc, AXE_178_CMD_READ_NODEID, 0, 0, sc->axe_enaddr);
	} else if (sc->axe_flags & AX772B) {
		axe_ax88772b_init(sc);
	} else
		axe_cmd(sc, AXE_172_CMD_READ_NODEID, 0, 0, sc->axe_enaddr);

	/*
	 * Fetch IPG values.
	 */
	if (sc->axe_flags & (AX772A | AX772B)) {
		/* Set IPG values. */
		sc->axe_ipgs[0] = AXE_IPG0_DEFAULT;
		sc->axe_ipgs[1] = AXE_IPG1_DEFAULT;
		sc->axe_ipgs[2] = AXE_IPG2_DEFAULT;
	} else
		axe_cmd(sc, AXE_CMD_READ_IPG012, 0, 0, sc->axe_ipgs);

	axe_unlock_mii(sc);

	/*
	 * An ASIX chip was detected. Inform the world.
	 */
	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->axe_enaddr));

	/* Initialize interface info.*/
	ifp = &sc->sc_if;
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, devname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = axe_ioctl;
	ifp->if_start = axe_start;
	ifp->if_init = axe_init;
	ifp->if_stop = axe_stop;
	ifp->if_watchdog = axe_watchdog;

	IFQ_SET_READY(&ifp->if_snd);

	if (AXE_IS_178_FAMILY(sc))
		sc->axe_ec.ec_capabilities = ETHERCAP_VLAN_MTU;
	if (sc->axe_flags & AX772B) {
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
	u_int adv_pause;
	if (sc->axe_flags & (AX772A | AX772B | AX178))
		adv_pause = MIIF_DOPAUSE;
	else
		adv_pause = 0;
	adv_pause = 0;

	/* Initialize MII/media info. */
	mii = &sc->axe_mii;
	mii->mii_ifp = ifp;
	mii->mii_readreg = axe_miibus_readreg;
	mii->mii_writereg = axe_miibus_writereg;
	mii->mii_statchg = axe_miibus_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;

	sc->axe_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);

	mii_attach(sc->axe_dev, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY,
	    adv_pause);

	if (LIST_EMPTY(&mii->mii_phys)) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->axe_enaddr);
	rnd_attach_source(&sc->rnd_source, device_xname(sc->axe_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	callout_init(&sc->axe_stat_ch, 0);
	callout_setfunc(&sc->axe_stat_ch, axe_tick, sc);

	sc->axe_attached = true;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->axe_udev, sc->axe_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

int
axe_detach(device_t self, int flags)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = device_private(self);
	int s;
	struct ifnet *ifp = &sc->sc_if;

	/* Detached before attached finished, so just bail out. */
	if (!sc->axe_attached)
		return 0;

	pmf_device_deregister(self);

	sc->axe_dying = true;

	if (sc->axe_ep[AXE_ENDPT_TX] != NULL)
		usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_TX]);
	if (sc->axe_ep[AXE_ENDPT_RX] != NULL)
		usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_RX]);
	if (sc->axe_ep[AXE_ENDPT_INTR] != NULL)
		usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_INTR]);

	/*
	 * Remove any pending tasks.  They cannot be executing because they run
	 * in the same thread as detach.
	 */
	usb_rem_task(sc->axe_udev, &sc->axe_tick_task);

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING)
		axe_stop(ifp, 1);


	if (--sc->axe_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_waitold(sc->axe_dev);
	}

	callout_destroy(&sc->axe_stat_ch);
	mutex_destroy(&sc->axe_mii_lock);
	rnd_detach_source(&sc->rnd_source);
	mii_detach(&sc->axe_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->axe_mii.mii_media, IFM_INST_ANY);
	ether_ifdetach(ifp);
	if_detach(ifp);

#ifdef DIAGNOSTIC
	if (sc->axe_ep[AXE_ENDPT_TX] != NULL ||
	    sc->axe_ep[AXE_ENDPT_RX] != NULL ||
	    sc->axe_ep[AXE_ENDPT_INTR] != NULL)
		aprint_debug_dev(self, "detach has active endpoints\n");
#endif

	sc->axe_attached = false;

	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->axe_udev, sc->axe_dev);

	return 0;
}

int
axe_activate(device_t self, devact_t act)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(&sc->axe_ec.ec_if);
		sc->axe_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static int
axe_rx_list_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();

	struct axe_cdata *cd;
	struct axe_chain *c;
	int i;

	cd = &sc->axe_cdata;
	for (i = 0; i < AXE_RX_LIST_CNT; i++) {
		c = &cd->axe_rx_chain[i];
		c->axe_sc = sc;
		c->axe_idx = i;
		if (c->axe_xfer == NULL) {
			int err = usbd_create_xfer(sc->axe_ep[AXE_ENDPT_RX],
			    sc->axe_bufsz, 0, 0, &c->axe_xfer);
			if (err)
				return err;
			c->axe_buf = usbd_get_buffer(c->axe_xfer);
		}
	}

	return 0;
}

static int
axe_tx_list_init(struct axe_softc *sc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_cdata *cd;
	struct axe_chain *c;
	int i;

	cd = &sc->axe_cdata;
	for (i = 0; i < AXE_TX_LIST_CNT; i++) {
		c = &cd->axe_tx_chain[i];
		c->axe_sc = sc;
		c->axe_idx = i;
		if (c->axe_xfer == NULL) {
			int err = usbd_create_xfer(sc->axe_ep[AXE_ENDPT_TX],
			    sc->axe_bufsz, USBD_FORCE_SHORT_XFER, 0,
			    &c->axe_xfer);
			if (err)
				return err;
			c->axe_buf = usbd_get_buffer(c->axe_xfer);
		}
	}

	return 0;
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
axe_rxeof(struct usbd_xfer *xfer, void * priv, usbd_status status)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc;
	struct axe_chain *c;
	struct ifnet *ifp;
	uint8_t *buf;
	uint32_t total_len;
	struct mbuf *m;
	int s;

	c = (struct axe_chain *)priv;
	sc = c->axe_sc;
	buf = c->axe_buf;
	ifp = &sc->sc_if;

	if (sc->axe_dying)
		return;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		if (usbd_ratecheck(&sc->axe_rx_notice)) {
			aprint_error_dev(sc->axe_dev, "usb errors on rx: %s\n",
			    usbd_errstr(status));
		}
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->axe_ep[AXE_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	do {
		u_int pktlen = 0;
		u_int rxlen = 0;
		int flags = 0;
		if ((sc->axe_flags & AXSTD_FRAME) != 0) {
			struct axe_sframe_hdr hdr;

			if (total_len < sizeof(hdr)) {
				ifp->if_ierrors++;
				goto done;
			}

			memcpy(&hdr, buf, sizeof(hdr));

			DPRINTFN(20, "total_len %#jx len %jx ilen %#jx",
			    total_len,
			    (le16toh(hdr.len) & AXE_RH1M_RXLEN_MASK),
			    (le16toh(hdr.ilen) & AXE_RH1M_RXLEN_MASK), 0);

			total_len -= sizeof(hdr);
			buf += sizeof(hdr);

			if (((le16toh(hdr.len) & AXE_RH1M_RXLEN_MASK) ^
			    (le16toh(hdr.ilen) & AXE_RH1M_RXLEN_MASK)) !=
			    AXE_RH1M_RXLEN_MASK) {
				ifp->if_ierrors++;
				goto done;
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

		} else if ((sc->axe_flags & AXCSUM_FRAME) != 0) {
			struct axe_csum_hdr csum_hdr;

			if (total_len <  sizeof(csum_hdr)) {
				ifp->if_ierrors++;
				goto done;
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
				ifp->if_ierrors++;
				DPRINTFN(20, "len %#jx ilen %#jx lenmask %#jx "
				    "err",
				    AXE_CSUM_RXBYTES(csum_hdr.len),
				    AXE_CSUM_RXBYTES(csum_hdr.ilen),
				    sc->sc_lenmask, 0);
				goto done;
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
				ifp->if_ierrors++;
				goto done;
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

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			ifp->if_ierrors++;
			goto done;
		}

		if (pktlen > MHLEN - ETHER_ALIGN) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				ifp->if_ierrors++;
				goto done;
			}
		}
		m->m_data += ETHER_ALIGN;

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = pktlen;
		m->m_pkthdr.csum_flags = flags;

		memcpy(mtod(m, uint8_t *), buf, pktlen);
		buf += rxlen;

		DPRINTFN(10, "deliver %jd (%#jx)", m->m_len, m->m_len, 0, 0);

		s = splnet();

		if_percpuq_enqueue((ifp)->if_percpuq, (m));

		splx(s);

	} while (total_len > 0);

 done:

	/* Setup new transfer. */
	usbd_setup_xfer(xfer, c, c->axe_buf, sc->axe_bufsz,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, axe_rxeof);
	usbd_transfer(xfer);

	DPRINTFN(10, "start rx", 0, 0, 0, 0);
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */

static void
axe_txeof(struct usbd_xfer *xfer, void * priv, usbd_status status)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_chain *c = priv;
	struct axe_softc *sc = c->axe_sc;
	struct ifnet *ifp = &sc->sc_if;
	int s;


	if (sc->axe_dying)
		return;

	s = splnet();

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		aprint_error_dev(sc->axe_dev, "usb error on tx: %s\n",
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->axe_ep[AXE_ENDPT_TX]);
		splx(s);
		return;
	}
	ifp->if_opackets++;

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		axe_start(ifp);

	splx(s);
}

static void
axe_tick(void *xsc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = xsc;

	if (sc == NULL)
		return;

	if (sc->axe_dying)
		return;

	/* Perform periodic stuff in process context */
	usb_add_task(sc->axe_udev, &sc->axe_tick_task, USB_TASKQ_DRIVER);
}

static void
axe_tick_task(void *xsc)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	int s;
	struct axe_softc *sc = xsc;
	struct ifnet *ifp;
	struct mii_data *mii;

	if (sc == NULL)
		return;

	if (sc->axe_dying)
		return;

	ifp = &sc->sc_if;
	mii = &sc->axe_mii;

	if (mii == NULL)
		return;

	s = splnet();

	mii_tick(mii);
	if (sc->axe_link == 0 &&
	    (mii->mii_media_status & IFM_ACTIVE) != 0 &&
	    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
		DPRINTF("got link", 0, 0, 0, 0);
		sc->axe_link++;
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			axe_start(ifp);
	}

	callout_schedule(&sc->axe_stat_ch, hz);

	splx(s);
}

static int
axe_encap(struct axe_softc *sc, struct mbuf *m, int idx)
{
	struct ifnet *ifp = &sc->sc_if;
	struct axe_chain *c;
	usbd_status err;
	int length, boundary;

	c = &sc->axe_cdata.axe_tx_chain[idx];

	/*
	 * Copy the mbuf data into a contiguous buffer, leaving two
	 * bytes at the beginning to hold the frame length.
	 */
	if (AXE_IS_178_FAMILY(sc)) {
	    	struct axe_sframe_hdr hdr;

		boundary = (sc->axe_udev->ud_speed == USB_SPEED_HIGH) ? 512 : 64;

		hdr.len = htole16(m->m_pkthdr.len);
		hdr.ilen = ~hdr.len;

		memcpy(c->axe_buf, &hdr, sizeof(hdr));
		length = sizeof(hdr);

		m_copydata(m, 0, m->m_pkthdr.len, c->axe_buf + length);
		length += m->m_pkthdr.len;

		if ((length % boundary) == 0) {
			hdr.len = 0x0000;
			hdr.ilen = 0xffff;
			memcpy(c->axe_buf + length, &hdr, sizeof(hdr));
			length += sizeof(hdr);
		}
	} else {
		m_copydata(m, 0, m->m_pkthdr.len, c->axe_buf);
		length = m->m_pkthdr.len;
	}

	usbd_setup_xfer(c->axe_xfer, c, c->axe_buf, length,
	    USBD_FORCE_SHORT_XFER, 10000, axe_txeof);

	/* Transmit */
	err = usbd_transfer(c->axe_xfer);
	if (err != USBD_IN_PROGRESS) {
		axe_stop(ifp, 0);
		return EIO;
	}

	sc->axe_cdata.axe_tx_cnt++;

	return 0;
}


static void
axe_csum_cfg(struct axe_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	uint16_t csum1, csum2;

	if ((sc->axe_flags & AX772B) != 0) {
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

static void
axe_start(struct ifnet *ifp)
{
	struct axe_softc *sc;
	struct mbuf *m;

	sc = ifp->if_softc;

	if ((ifp->if_flags & (IFF_OACTIVE|IFF_RUNNING)) != IFF_RUNNING)
		return;

	IFQ_POLL(&ifp->if_snd, m);
	if (m == NULL) {
		return;
	}

	if (axe_encap(sc, m, 0)) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}
	IFQ_DEQUEUE(&ifp->if_snd, m);

	/*
	 * If there's a BPF listener, bounce a copy of this frame
	 * to him.
	 */
	bpf_mtap(ifp, m);
	m_freem(m);

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;

	return;
}

static int
axe_init(struct ifnet *ifp)
{
	AXEHIST_FUNC(); AXEHIST_CALLED();
	struct axe_softc *sc = ifp->if_softc;
	struct axe_chain *c;
	usbd_status err;
	int rxmode;
	int i, s;

	s = splnet();

	if (ifp->if_flags & IFF_RUNNING)
		axe_stop(ifp, 0);

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
	axe_reset(sc);

	axe_lock_mii(sc);

#if 0
	ret = asix_write_gpio(dev, AX_GPIO_RSE | AX_GPIO_GPO_2 |
			      AX_GPIO_GPO2EN, 5, in_pm);
#endif
	/* Set MAC address and transmitter IPG values. */
	if (AXE_IS_178_FAMILY(sc)) {
		axe_cmd(sc, AXE_178_CMD_WRITE_NODEID, 0, 0, sc->axe_enaddr);
		axe_cmd(sc, AXE_178_CMD_WRITE_IPG012, sc->axe_ipgs[2],
		    (sc->axe_ipgs[1] << 8) | (sc->axe_ipgs[0]), NULL);
	} else {
		axe_cmd(sc, AXE_172_CMD_WRITE_NODEID, 0, 0, sc->axe_enaddr);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG0, 0, sc->axe_ipgs[0], NULL);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG1, 0, sc->axe_ipgs[1], NULL);
		axe_cmd(sc, AXE_172_CMD_WRITE_IPG2, 0, sc->axe_ipgs[2], NULL);
	}
	if (AXE_IS_178_FAMILY(sc)) {
		sc->axe_flags &= ~(AXSTD_FRAME | AXCSUM_FRAME);
		if ((sc->axe_flags & AX772B) != 0 &&
		    (ifp->if_capenable & AX_RXCSUM) != 0) {
			sc->sc_lenmask = AXE_CSUM_HDR_LEN_MASK;
			sc->axe_flags |= AXCSUM_FRAME;
		} else {
			sc->sc_lenmask = AXE_HDR_LEN_MASK;
			sc->axe_flags |= AXSTD_FRAME;
		}
	}

	/* Configure TX/RX checksum offloading. */
	axe_csum_cfg(sc);

	if (sc->axe_flags & AX772B) {
		/* AX88772B uses different maximum frame burst configuration. */
		axe_cmd(sc, AXE_772B_CMD_RXCTL_WRITE_CFG,
		    ax88772b_mfb_table[AX88772B_MFB_16K].threshold,
		    ax88772b_mfb_table[AX88772B_MFB_16K].byte_cnt, NULL);
	}
	/* Enable receiver, set RX mode */
	rxmode = (AXE_RXCMD_MULTICAST | AXE_RXCMD_ENABLE);
	if (AXE_IS_178_FAMILY(sc)) {
		if (sc->axe_flags & AX772B) {
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
			if (sc->axe_flags & AXCSUM_FRAME)
				rxmode |= AXE_772B_RXCMD_IPHDR_ALIGN;
		} else {
			/*
			 * Default Rx buffer size is too small to get
			 * maximum performance.
			 */
#if 0
			if (sc->axe_udev->ud_speed == USB_SPEED_HIGH) {
				/* Largest possible USB buffer size for AX88178 */
#endif
			rxmode |= AXE_178_RXCMD_MFB_16384;
		}
	} else {
		rxmode |= AXE_172_RXCMD_UNICAST;
	}


	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC)
		rxmode |= AXE_RXCMD_PROMISC;

	if (ifp->if_flags & IFF_BROADCAST)
		rxmode |= AXE_RXCMD_BROADCAST;

	DPRINTF("rxmode 0x%#jx", rxmode, 0, 0, 0);

	axe_cmd(sc, AXE_CMD_RXCTL_WRITE, 0, rxmode, NULL);
	axe_unlock_mii(sc);

	/* Load the multicast filter. */
	axe_setmulti(sc);

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->axe_iface, sc->axe_ed[AXE_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->axe_ep[AXE_ENDPT_RX]);
	if (err) {
		aprint_error_dev(sc->axe_dev, "open rx pipe failed: %s\n",
		    usbd_errstr(err));
		splx(s);
		return EIO;
	}

	err = usbd_open_pipe(sc->axe_iface, sc->axe_ed[AXE_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->axe_ep[AXE_ENDPT_TX]);
	if (err) {
		aprint_error_dev(sc->axe_dev, "open tx pipe failed: %s\n",
		    usbd_errstr(err));
		splx(s);
		return EIO;
	}

	/* Init RX ring. */
	if (axe_rx_list_init(sc) != 0) {
		aprint_error_dev(sc->axe_dev, "rx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	/* Init TX ring. */
	if (axe_tx_list_init(sc) != 0) {
		aprint_error_dev(sc->axe_dev, "tx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	/* Start up the receive pipe. */
	for (i = 0; i < AXE_RX_LIST_CNT; i++) {
		c = &sc->axe_cdata.axe_rx_chain[i];
		usbd_setup_xfer(c->axe_xfer, c, c->axe_buf, sc->axe_bufsz,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, axe_rxeof);
		usbd_transfer(c->axe_xfer);
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	callout_schedule(&sc->axe_stat_ch, hz);
	return 0;
}

static int
axe_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct axe_softc *sc = ifp->if_softc;
	int s;
	int error = 0;

	s = splnet();

	switch(cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;

		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			axe_stop(ifp, 1);
			break;
		case IFF_UP:
			axe_init(ifp);
			break;
		case IFF_UP | IFF_RUNNING:
			if ((ifp->if_flags ^ sc->axe_if_flags) == IFF_PROMISC)
				axe_setmulti(sc);
			else
				axe_init(ifp);
			break;
		}
		sc->axe_if_flags = ifp->if_flags;
		break;

	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;

		error = 0;

		if (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI)
			axe_setmulti(sc);

	}
	splx(s);

	return error;
}

static void
axe_watchdog(struct ifnet *ifp)
{
	struct axe_softc *sc;
	struct axe_chain *c;
	usbd_status stat;
	int s;

	sc = ifp->if_softc;

	ifp->if_oerrors++;
	aprint_error_dev(sc->axe_dev, "watchdog timeout\n");

	s = splusb();
	c = &sc->axe_cdata.axe_tx_chain[0];
	usbd_get_xfer_status(c->axe_xfer, NULL, NULL, NULL, &stat);
	axe_txeof(c->axe_xfer, c, stat);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		axe_start(ifp);
	splx(s);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
axe_stop(struct ifnet *ifp, int disable)
{
	struct axe_softc *sc = ifp->if_softc;
	usbd_status err;
	int i;

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	callout_stop(&sc->axe_stat_ch);

	/* Stop transfers. */
	if (sc->axe_ep[AXE_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_RX]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "abort rx pipe failed: %s\n", usbd_errstr(err));
		}
	}

	if (sc->axe_ep[AXE_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_TX]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "abort tx pipe failed: %s\n", usbd_errstr(err));
		}
	}

	if (sc->axe_ep[AXE_ENDPT_INTR] != NULL) {
		err = usbd_abort_pipe(sc->axe_ep[AXE_ENDPT_INTR]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "abort intr pipe failed: %s\n", usbd_errstr(err));
		}
	}

	axe_reset(sc);

	/* Free RX resources. */
	for (i = 0; i < AXE_RX_LIST_CNT; i++) {
		if (sc->axe_cdata.axe_rx_chain[i].axe_xfer != NULL) {
			usbd_destroy_xfer(sc->axe_cdata.axe_rx_chain[i].axe_xfer);
			sc->axe_cdata.axe_rx_chain[i].axe_xfer = NULL;
		}
	}

	/* Free TX resources. */
	for (i = 0; i < AXE_TX_LIST_CNT; i++) {
		if (sc->axe_cdata.axe_tx_chain[i].axe_xfer != NULL) {
			usbd_destroy_xfer(sc->axe_cdata.axe_tx_chain[i].axe_xfer);
			sc->axe_cdata.axe_tx_chain[i].axe_xfer = NULL;
		}
	}

	/* Close pipes. */
	if (sc->axe_ep[AXE_ENDPT_RX] != NULL) {
		err = usbd_close_pipe(sc->axe_ep[AXE_ENDPT_RX]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "close rx pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axe_ep[AXE_ENDPT_RX] = NULL;
	}

	if (sc->axe_ep[AXE_ENDPT_TX] != NULL) {
		err = usbd_close_pipe(sc->axe_ep[AXE_ENDPT_TX]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "close tx pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axe_ep[AXE_ENDPT_TX] = NULL;
	}

	if (sc->axe_ep[AXE_ENDPT_INTR] != NULL) {
		err = usbd_close_pipe(sc->axe_ep[AXE_ENDPT_INTR]);
		if (err) {
			aprint_error_dev(sc->axe_dev,
			    "close intr pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axe_ep[AXE_ENDPT_INTR] = NULL;
	}

	sc->axe_link = 0;
}

MODULE(MODULE_CLASS_DRIVER, if_axe, "bpf");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
if_axe_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_axe,
		    cfattach_ioconf_axe, cfdata_ioconf_axe);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_axe,
		    cfattach_ioconf_axe, cfdata_ioconf_axe);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
