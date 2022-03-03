/*	$NetBSD: if_smsc.c,v 1.91 2022/03/03 05:56:09 riastradh Exp $	*/

/*	$OpenBSD: if_smsc.c,v 1.4 2012/09/27 12:38:11 jsg Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/net/if_smsc.c,v 1.1 2012/08/15 04:03:55 gonzo Exp $ */
/*-
 * Copyright (c) 2012
 *	Ben Gray <bgray@freebsd.org>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SMSC LAN9xxx devices (http://www.smsc.com/)
 *
 * The LAN9500 & LAN9500A devices are stand-alone USB to Ethernet chips that
 * support USB 2.0 and 10/100 Mbps Ethernet.
 *
 * The LAN951x devices are an integrated USB hub and USB to Ethernet adapter.
 * The driver only covers the Ethernet part, the standard USB hub driver
 * supports the hub part.
 *
 * This driver is closely modelled on the Linux driver written and copyrighted
 * by SMSC.
 *
 * H/W TCP & UDP Checksum Offloading
 * ---------------------------------
 * The chip supports both tx and rx offloading of UDP & TCP checksums, this
 * feature can be dynamically enabled/disabled.
 *
 * RX checksuming is performed across bytes after the IPv4 header to the end of
 * the Ethernet frame, this means if the frame is padded with non-zero values
 * the H/W checksum will be incorrect, however the rx code compensates for this.
 *
 * TX checksuming is more complicated, the device requires a special header to
 * be prefixed onto the start of the frame which indicates the start and end
 * positions of the UDP or TCP frame.  This requires the driver to manually
 * go through the packet data and decode the headers prior to sending.
 * On Linux they generally provide cues to the location of the csum and the
 * area to calculate it over, on FreeBSD we seem to have to do it all ourselves,
 * hence this is not as optimal and therefore h/w TX checksum is currently not
 * implemented.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_smsc.c,v 1.91 2022/03/03 05:56:09 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/if_smscreg.h>

#include "ioconf.h"

struct smsc_softc {
	struct usbnet		smsc_un;

	/*
	 * The following stores the settings in the mac control (MAC_CSR)
	 * register
	 */
	uint32_t		sc_mac_csr;
	uint32_t		sc_rev_id;

	uint32_t		sc_coe_ctrl;
};

#define SMSC_MIN_BUFSZ		2048
#define SMSC_MAX_BUFSZ		18944

/*
 * Various supported device vendors/products.
 */
static const struct usb_devno smsc_devs[] = {
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_LAN89530 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_LAN9530 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_LAN9730 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500A },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500A_ALT },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500A_HAL },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500A_SAL10 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500_ALT },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9500_SAL10 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9505 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9505A },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9505A_HAL },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9505A_SAL10 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9505_SAL10 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9512_14 },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9512_14_ALT },
	{ USB_VENDOR_SMSC,	USB_PRODUCT_SMSC_SMSC9512_14_SAL10 }
};

#ifdef USB_DEBUG
#ifndef USMSC_DEBUG
#define usmscdebug 0
#else
static int usmscdebug = 1;

SYSCTL_SETUP(sysctl_hw_smsc_setup, "sysctl hw.usmsc setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "usmsc",
	    SYSCTL_DESCR("usmsc global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &usmscdebug, sizeof(usmscdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* SMSC_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(usmscdebug,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usmscdebug,N,FMT,A,B,C,D)
#define USMSCHIST_FUNC()	USBHIST_FUNC()
#define USMSCHIST_CALLED()	USBHIST_CALLED(usmscdebug)

#define smsc_warn_printf(un, fmt, args...) \
	printf("%s: warning: " fmt, device_xname((un)->un_dev), ##args)

#define smsc_err_printf(un, fmt, args...) \
	printf("%s: error: " fmt, device_xname((un)->un_dev), ##args)

/* Function declarations */
static int	 smsc_match(device_t, cfdata_t, void *);
static void	 smsc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(usmsc, sizeof(struct smsc_softc),
    smsc_match, smsc_attach, usbnet_detach, usbnet_activate);

static int	 smsc_chip_init(struct usbnet *);
static int	 smsc_setmacaddress(struct usbnet *, const uint8_t *);

static int	 smsc_uno_init(struct ifnet *);
static void	 smsc_uno_stop(struct ifnet *, int);

static void	 smsc_reset(struct smsc_softc *);

static void	 smsc_uno_miibus_statchg(struct ifnet *);
static int	 smsc_readreg(struct usbnet *, uint32_t, uint32_t *);
static int	 smsc_writereg(struct usbnet *, uint32_t, uint32_t);
static int	 smsc_wait_for_bits(struct usbnet *, uint32_t, uint32_t);
static int	 smsc_uno_miibus_readreg(struct usbnet *, int, int, uint16_t *);
static int	 smsc_uno_miibus_writereg(struct usbnet *, int, int, uint16_t);

static int	 smsc_uno_ioctl(struct ifnet *, u_long, void *);
static void	 smsc_uno_mcast(struct ifnet *);
static unsigned	 smsc_uno_tx_prepare(struct usbnet *, struct mbuf *,
		     struct usbnet_chain *);
static void	 smsc_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
		     uint32_t);

static const struct usbnet_ops smsc_ops = {
	.uno_stop = smsc_uno_stop,
	.uno_ioctl = smsc_uno_ioctl,
	.uno_mcast = smsc_uno_mcast,
	.uno_read_reg = smsc_uno_miibus_readreg,
	.uno_write_reg = smsc_uno_miibus_writereg,
	.uno_statchg = smsc_uno_miibus_statchg,
	.uno_tx_prepare = smsc_uno_tx_prepare,
	.uno_rx_loop = smsc_uno_rx_loop,
	.uno_init = smsc_uno_init,
};

static int
smsc_readreg(struct usbnet *un, uint32_t off, uint32_t *data)
{
	usb_device_request_t req;
	uint32_t buf;
	usbd_status err;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = SMSC_UR_READ_REG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, off);
	USETW(req.wLength, 4);

	err = usbd_do_request(un->un_udev, &req, &buf);
	if (err != 0)
		smsc_warn_printf(un, "Failed to read register 0x%0x\n", off);

	*data = le32toh(buf);

	return err;
}

static int
smsc_writereg(struct usbnet *un, uint32_t off, uint32_t data)
{
	usb_device_request_t req;
	uint32_t buf;
	usbd_status err;

	if (usbnet_isdying(un))
		return 0;

	buf = htole32(data);

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = SMSC_UR_WRITE_REG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, off);
	USETW(req.wLength, 4);

	err = usbd_do_request(un->un_udev, &req, &buf);
	if (err != 0)
		smsc_warn_printf(un, "Failed to write register 0x%0x\n", off);

	return err;
}

static int
smsc_wait_for_bits(struct usbnet *un, uint32_t reg, uint32_t bits)
{
	uint32_t val;
	int err, i;

	for (i = 0; i < 100; i++) {
		if (usbnet_isdying(un))
			return ENXIO;
		if ((err = smsc_readreg(un, reg, &val)) != 0)
			return err;
		if (!(val & bits))
			return 0;
		DELAY(5);
	}

	return 1;
}

static int
smsc_uno_miibus_readreg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	uint32_t addr;
	uint32_t data = 0;

	if (un->un_phyno != phy) {
		*val = 0;
		return EINVAL;
	}

	if (smsc_wait_for_bits(un, SMSC_MII_ADDR, SMSC_MII_BUSY) != 0) {
		smsc_warn_printf(un, "MII is busy\n");
		*val = 0;
		return ETIMEDOUT;
	}

	addr = (phy << 11) | (reg << 6) | SMSC_MII_READ;
	smsc_writereg(un, SMSC_MII_ADDR, addr);

	if (smsc_wait_for_bits(un, SMSC_MII_ADDR, SMSC_MII_BUSY) != 0) {
		smsc_warn_printf(un, "MII read timeout\n");
		*val = 0;
		return ETIMEDOUT;
	}

	smsc_readreg(un, SMSC_MII_DATA, &data);

	*val = data & 0xffff;
	return 0;
}

static int
smsc_uno_miibus_writereg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	uint32_t addr;

	if (un->un_phyno != phy)
		return EINVAL;

	if (smsc_wait_for_bits(un, SMSC_MII_ADDR, SMSC_MII_BUSY) != 0) {
		smsc_warn_printf(un, "MII is busy\n");
		return ETIMEDOUT;
	}

	smsc_writereg(un, SMSC_MII_DATA, val);

	addr = (phy << 11) | (reg << 6) | SMSC_MII_WRITE;
	smsc_writereg(un, SMSC_MII_ADDR, addr);

	if (smsc_wait_for_bits(un, SMSC_MII_ADDR, SMSC_MII_BUSY) != 0) {
		smsc_warn_printf(un, "MII write timeout\n");
		return ETIMEDOUT;
	}

	return 0;
}

static void
smsc_uno_miibus_statchg(struct ifnet *ifp)
{
	USMSCHIST_FUNC(); USMSCHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;

	if (usbnet_isdying(un))
		return;

	struct smsc_softc * const sc = usbnet_softc(un);
	struct mii_data * const mii = usbnet_mii(un);
	uint32_t flow;
	uint32_t afc_cfg;

	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
			case IFM_10_T:
			case IFM_100_TX:
				usbnet_set_link(un, true);
				break;
			case IFM_1000_T:
				/* Gigabit ethernet not supported by chipset */
				break;
			default:
				break;
		}
	}

	/* Lost link, do nothing. */
	if (!usbnet_havelink(un))
		return;

	int err = smsc_readreg(un, SMSC_AFC_CFG, &afc_cfg);
	if (err) {
		smsc_warn_printf(un, "failed to read initial AFC_CFG, "
		    "error %d\n", err);
		return;
	}

	/* Enable/disable full duplex operation and TX/RX pause */
	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0) {
		DPRINTF("full duplex operation", 0, 0, 0, 0);
		sc->sc_mac_csr &= ~SMSC_MAC_CSR_RCVOWN;
		sc->sc_mac_csr |= SMSC_MAC_CSR_FDPX;

		if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_RXPAUSE) != 0)
			flow = 0xffff0002;
		else
			flow = 0;

		if ((IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_TXPAUSE) != 0)
			afc_cfg |= 0xf;
		else
			afc_cfg &= ~0xf;
	} else {
		DPRINTF("half duplex operation", 0, 0, 0, 0);
		sc->sc_mac_csr &= ~SMSC_MAC_CSR_FDPX;
		sc->sc_mac_csr |= SMSC_MAC_CSR_RCVOWN;

		flow = 0;
		afc_cfg |= 0xf;
	}

	err = smsc_writereg(un, SMSC_MAC_CSR, sc->sc_mac_csr);
	err += smsc_writereg(un, SMSC_FLOW, flow);
	err += smsc_writereg(un, SMSC_AFC_CFG, afc_cfg);

	if (err)
		smsc_warn_printf(un, "media change failed, error %d\n", err);
}

static inline uint32_t
smsc_hash(uint8_t addr[ETHER_ADDR_LEN])
{

	return (ether_crc32_be(addr, ETHER_ADDR_LEN) >> 26) & 0x3f;
}

static void
smsc_uno_mcast(struct ifnet *ifp)
{
	USMSCHIST_FUNC(); USMSCHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	struct smsc_softc * const sc = usbnet_softc(un);
	struct ethercom *ec = usbnet_ec(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t hashtbl[2] = { 0, 0 };
	uint32_t hash;

	if (usbnet_isdying(un))
		return;

	if (ifp->if_flags & IFF_PROMISC) {
		ETHER_LOCK(ec);
allmulti:
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		DPRINTF("receive all multicast enabled", 0, 0, 0, 0);
		sc->sc_mac_csr |= SMSC_MAC_CSR_MCPAS;
		sc->sc_mac_csr &= ~SMSC_MAC_CSR_HPFILT;
		smsc_writereg(un, SMSC_MAC_CSR, sc->sc_mac_csr);
		return;
	} else {
		sc->sc_mac_csr |= SMSC_MAC_CSR_HPFILT;
		sc->sc_mac_csr &= ~(SMSC_MAC_CSR_PRMS | SMSC_MAC_CSR_MCPAS);
	}

	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			goto allmulti;
		}

		hash = smsc_hash(enm->enm_addrlo);
		hashtbl[hash >> 5] |= 1 << (hash & 0x1F);
		ETHER_NEXT_MULTI(step, enm);
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_UNLOCK(ec);

	/* Debug */
	if (sc->sc_mac_csr & SMSC_MAC_CSR_HPFILT) {
		DPRINTF("receive select group of macs", 0, 0, 0, 0);
	} else {
		DPRINTF("receive own packets only", 0, 0, 0, 0);
	}

	/* Write the hash table and mac control registers */

	//XXX should we be doing this?
	smsc_writereg(un, SMSC_HASHH, hashtbl[1]);
	smsc_writereg(un, SMSC_HASHL, hashtbl[0]);
	smsc_writereg(un, SMSC_MAC_CSR, sc->sc_mac_csr);
}

static int
smsc_setoe_locked(struct usbnet *un)
{
	struct smsc_softc * const sc = usbnet_softc(un);
	struct ifnet * const ifp = usbnet_ifp(un);
	uint32_t val;
	int err;

	KASSERT(IFNET_LOCKED(ifp));

	err = smsc_readreg(un, SMSC_COE_CTRL, &val);
	if (err != 0) {
		smsc_warn_printf(un, "failed to read SMSC_COE_CTRL (err=%d)\n",
		    err);
		return err;
	}

	/* Enable/disable the Rx checksum */
	if (ifp->if_capenable & (IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx))
		val |= (SMSC_COE_CTRL_RX_EN | SMSC_COE_CTRL_RX_MODE);
	else
		val &= ~(SMSC_COE_CTRL_RX_EN | SMSC_COE_CTRL_RX_MODE);

	/* Enable/disable the Tx checksum (currently not supported) */
	if (ifp->if_capenable & (IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx))
		val |= SMSC_COE_CTRL_TX_EN;
	else
		val &= ~SMSC_COE_CTRL_TX_EN;

	sc->sc_coe_ctrl = val;

	err = smsc_writereg(un, SMSC_COE_CTRL, val);
	if (err != 0) {
		smsc_warn_printf(un, "failed to write SMSC_COE_CTRL (err=%d)\n",
		    err);
		return err;
	}

	return 0;
}

static int
smsc_setmacaddress(struct usbnet *un, const uint8_t *addr)
{
	USMSCHIST_FUNC(); USMSCHIST_CALLED();
	int err;
	uint32_t val;

	DPRINTF("setting mac address to %02jx:%02jx:%02jx:...", addr[0],
	    addr[1], addr[2], 0);

	DPRINTF("... %02jx:%02jx:%02jx", addr[3], addr[4], addr[5], 0);

	val = ((uint32_t)addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8)
	    | addr[0];
	if ((err = smsc_writereg(un, SMSC_MAC_ADDRL, val)) != 0)
		goto done;

	val = (addr[5] << 8) | addr[4];
	err = smsc_writereg(un, SMSC_MAC_ADDRH, val);

done:
	return err;
}

static void
smsc_reset(struct smsc_softc *sc)
{
	struct usbnet * const un = &sc->smsc_un;

	if (usbnet_isdying(un))
		return;

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);

	/* Reinitialize controller to achieve full reset. */
	smsc_chip_init(un);
}

static int
smsc_uno_init(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct smsc_softc * const sc = usbnet_softc(un);

	/* Reset the ethernet interface. */
	smsc_reset(sc);

	/* TCP/UDP checksum offload engines. */
	smsc_setoe_locked(un);

	return 0;
}

static void
smsc_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;
	struct smsc_softc * const sc = usbnet_softc(un);

	// XXXNH didn't do this before
	smsc_reset(sc);
}

static int
smsc_chip_init(struct usbnet *un)
{
	struct smsc_softc * const sc = usbnet_softc(un);
	uint32_t reg_val;
	int burst_cap;
	int err;

	/* Enter H/W config mode */
	smsc_writereg(un, SMSC_HW_CFG, SMSC_HW_CFG_LRST);

	if ((err = smsc_wait_for_bits(un, SMSC_HW_CFG,
	    SMSC_HW_CFG_LRST)) != 0) {
		smsc_warn_printf(un, "timed-out waiting for reset to "
		    "complete\n");
		goto init_failed;
	}

	/* Reset the PHY */
	smsc_writereg(un, SMSC_PM_CTRL, SMSC_PM_CTRL_PHY_RST);

	if ((err = smsc_wait_for_bits(un, SMSC_PM_CTRL,
	    SMSC_PM_CTRL_PHY_RST)) != 0) {
		smsc_warn_printf(un, "timed-out waiting for phy reset to "
		    "complete\n");
		goto init_failed;
	}
	usbd_delay_ms(un->un_udev, 40);

	/* Set the mac address */
	struct ifnet * const ifp = usbnet_ifp(un);
	const char *eaddr = CLLADDR(ifp->if_sadl);
	if ((err = smsc_setmacaddress(un, eaddr)) != 0) {
		smsc_warn_printf(un, "failed to set the MAC address\n");
		goto init_failed;
	}

	/*
	 * Don't know what the HW_CFG_BIR bit is, but following the reset
	 * sequence as used in the Linux driver.
	 */
	if ((err = smsc_readreg(un, SMSC_HW_CFG, &reg_val)) != 0) {
		smsc_warn_printf(un, "failed to read HW_CFG: %d\n", err);
		goto init_failed;
	}
	reg_val |= SMSC_HW_CFG_BIR;
	smsc_writereg(un, SMSC_HW_CFG, reg_val);

	/*
	 * There is a so called 'turbo mode' that the linux driver supports, it
	 * seems to allow you to jam multiple frames per Rx transaction.
	 * By default this driver supports that and therefore allows multiple
	 * frames per USB transfer.
	 *
	 * The xfer buffer size needs to reflect this as well, therefore based
	 * on the calculations in the Linux driver the RX bufsize is set to
	 * 18944,
	 *     bufsz = (16 * 1024 + 5 * 512)
	 *
	 * Burst capability is the number of URBs that can be in a burst of
	 * data/ethernet frames.
	 */

	if (un->un_udev->ud_speed == USB_SPEED_HIGH)
		burst_cap = 37;
	else
		burst_cap = 128;

	smsc_writereg(un, SMSC_BURST_CAP, burst_cap);

	/* Set the default bulk in delay (magic value from Linux driver) */
	smsc_writereg(un, SMSC_BULK_IN_DLY, 0x00002000);

	/*
	 * Initialise the RX interface
	 */
	if ((err = smsc_readreg(un, SMSC_HW_CFG, &reg_val)) < 0) {
		smsc_warn_printf(un, "failed to read HW_CFG: (err = %d)\n",
		    err);
		goto init_failed;
	}

	/*
	 * The following settings are used for 'turbo mode', a.k.a multiple
	 * frames per Rx transaction (again info taken form Linux driver).
	 */
	reg_val |= (SMSC_HW_CFG_MEF | SMSC_HW_CFG_BCE);

	/*
	 * set Rx data offset to ETHER_ALIGN which will make the IP header
	 * align on a word boundary.
	 */
	reg_val |= ETHER_ALIGN << SMSC_HW_CFG_RXDOFF_SHIFT;

	smsc_writereg(un, SMSC_HW_CFG, reg_val);

	/* Clear the status register ? */
	smsc_writereg(un, SMSC_INTR_STATUS, 0xffffffff);

	/* Read and display the revision register */
	if ((err = smsc_readreg(un, SMSC_ID_REV, &sc->sc_rev_id)) < 0) {
		smsc_warn_printf(un, "failed to read ID_REV (err = %d)\n", err);
		goto init_failed;
	}

	/* GPIO/LED setup */
	reg_val = SMSC_LED_GPIO_CFG_SPD_LED | SMSC_LED_GPIO_CFG_LNK_LED |
	    SMSC_LED_GPIO_CFG_FDX_LED;
	smsc_writereg(un, SMSC_LED_GPIO_CFG, reg_val);

	/*
	 * Initialise the TX interface
	 */
	smsc_writereg(un, SMSC_FLOW, 0);

	smsc_writereg(un, SMSC_AFC_CFG, AFC_CFG_DEFAULT);

	/* Read the current MAC configuration */
	if ((err = smsc_readreg(un, SMSC_MAC_CSR, &sc->sc_mac_csr)) < 0) {
		smsc_warn_printf(un, "failed to read MAC_CSR (err=%d)\n", err);
		goto init_failed;
	}

	/* disable pad stripping, collides with checksum offload */
	sc->sc_mac_csr &= ~SMSC_MAC_CSR_PADSTR;

	/* Vlan */
	smsc_writereg(un, SMSC_VLAN1, (uint32_t)ETHERTYPE_VLAN);

	/*
	 * Start TX
	 */
	sc->sc_mac_csr |= SMSC_MAC_CSR_TXEN;
	smsc_writereg(un, SMSC_MAC_CSR, sc->sc_mac_csr);
	smsc_writereg(un, SMSC_TX_CFG, SMSC_TX_CFG_ON);

	/*
	 * Start RX
	 */
	sc->sc_mac_csr |= SMSC_MAC_CSR_RXEN;
	smsc_writereg(un, SMSC_MAC_CSR, sc->sc_mac_csr);

	return 0;

init_failed:
	smsc_err_printf(un, "smsc_chip_init failed (err=%d)\n", err);
	return err;
}

static int
smsc_uno_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct usbnet * const un = ifp->if_softc;

	switch (cmd) {
	case SIOCSIFCAP:
		smsc_setoe_locked(un);
		break;
	default:
		break;
	}

	return 0;
}

static int
smsc_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return (usb_lookup(smsc_devs, uaa->uaa_vendor, uaa->uaa_product) != NULL) ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
smsc_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct smsc_softc * const sc = device_private(self);
	struct usbnet * const un = &sc->smsc_un;
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	unsigned bufsz;
	int err, i;
	uint32_t mac_h, mac_l;

	KASSERT((void *)sc == un);

	aprint_naive("\n");
	aprint_normal("\n");

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &smsc_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = SMSC_RX_LIST_CNT;
	un->un_tx_list_cnt = SMSC_TX_LIST_CNT;

	devinfop = usbd_devinfo_alloc(un->un_udev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, SMSC_CONFIG_INDEX, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	/* Setup the endpoints for the SMSC LAN95xx device(s) */
	err = usbd_device2interface_handle(dev, SMSC_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	id = usbd_get_interface_descriptor(un->un_iface);

	if (dev->ud_speed >= USB_SPEED_HIGH) {
		bufsz = SMSC_MAX_BUFSZ;
	} else {
		bufsz = SMSC_MIN_BUFSZ;
	}
	un->un_rx_bufsz = bufsz;
	un->un_tx_bufsz = bufsz;

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
#if 0 /* not used yet */
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
#endif
		}
	}

	usbnet_attach(un, "smscdet");

#ifdef notyet
	/*
	 * We can do TCPv4, and UDPv4 checksums in hardware.
	 */
	struct ifnet *ifp = usbnet_ifp(un);

	ifp->if_capabilities |=
	    /*IFCAP_CSUM_TCPv4_Tx |*/ IFCAP_CSUM_TCPv4_Rx |
	    /*IFCAP_CSUM_UDPv4_Tx |*/ IFCAP_CSUM_UDPv4_Rx;
#endif
	struct ethercom *ec = usbnet_ec(un);
	ec->ec_capabilities = ETHERCAP_VLAN_MTU;

	/* Setup some of the basics */
	un->un_phyno = 1;

	/*
	 * Attempt to get the mac address, if an EEPROM is not attached this
	 * will just return FF:FF:FF:FF:FF:FF, so in such cases we invent a MAC
	 * address based on urandom.
	 */
	memset(un->un_eaddr, 0xff, ETHER_ADDR_LEN);

	prop_dictionary_t dict = device_properties(self);
	prop_data_t eaprop = prop_dictionary_get(dict, "mac-address");

	if (eaprop != NULL) {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(un->un_eaddr, prop_data_value(eaprop),
		    ETHER_ADDR_LEN);
	} else {
		/* Check if there is already a MAC address in the register */
		if ((smsc_readreg(un, SMSC_MAC_ADDRL, &mac_l) == 0) &&
		    (smsc_readreg(un, SMSC_MAC_ADDRH, &mac_h) == 0)) {
			un->un_eaddr[5] = (uint8_t)((mac_h >> 8) & 0xff);
			un->un_eaddr[4] = (uint8_t)((mac_h) & 0xff);
			un->un_eaddr[3] = (uint8_t)((mac_l >> 24) & 0xff);
			un->un_eaddr[2] = (uint8_t)((mac_l >> 16) & 0xff);
			un->un_eaddr[1] = (uint8_t)((mac_l >> 8) & 0xff);
			un->un_eaddr[0] = (uint8_t)((mac_l) & 0xff);
		}
	}

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static void
smsc_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	USMSCHIST_FUNC(); USMSCHIST_CALLED();
	struct smsc_softc * const sc = usbnet_softc(un);
	struct ifnet *ifp = usbnet_ifp(un);
	uint8_t *buf = c->unc_buf;
	int count;

	count = 0;
	DPRINTF("total_len %jd/%#jx", total_len, total_len, 0, 0);
	while (total_len != 0) {
		uint32_t rxhdr;
		if (total_len < sizeof(rxhdr)) {
			DPRINTF("total_len %jd < sizeof(rxhdr) %jd",
			    total_len, sizeof(rxhdr), 0, 0);
			if_statinc(ifp, if_ierrors);
			return;
		}

		memcpy(&rxhdr, buf, sizeof(rxhdr));
		rxhdr = le32toh(rxhdr);
		buf += sizeof(rxhdr);
		total_len -= sizeof(rxhdr);

		if (rxhdr & SMSC_RX_STAT_COLLISION)
			if_statinc(ifp, if_collisions);

		if (rxhdr & (SMSC_RX_STAT_ERROR
			   | SMSC_RX_STAT_LENGTH_ERROR
			   | SMSC_RX_STAT_MII_ERROR)) {
			DPRINTF("rx error (hdr 0x%08jx)", rxhdr, 0, 0, 0);
			if_statinc(ifp, if_ierrors);
			return;
		}

		uint16_t pktlen = (uint16_t)SMSC_RX_STAT_FRM_LENGTH(rxhdr);
		DPRINTF("total_len %jd pktlen %jd rxhdr 0x%08jx", total_len,
		    pktlen, rxhdr, 0);

		if (pktlen < ETHER_HDR_LEN) {
			DPRINTF("pktlen %jd < ETHER_HDR_LEN %jd", pktlen,
			    ETHER_HDR_LEN, 0, 0);
			if_statinc(ifp, if_ierrors);
			return;
		}

		pktlen += ETHER_ALIGN;

		if (pktlen > MCLBYTES) {
			DPRINTF("pktlen %jd > MCLBYTES %jd", pktlen, MCLBYTES, 0,
			    0);
			if_statinc(ifp, if_ierrors);
			return;
		}

		if (pktlen > total_len) {
			DPRINTF("pktlen %jd > total_len %jd", pktlen, total_len,
			    0, 0);
			if_statinc(ifp, if_ierrors);
			return;
		}

		uint8_t *pktbuf = buf + ETHER_ALIGN;
		size_t buflen = pktlen - ETHER_ALIGN;
		int mbuf_flags = M_HASFCS;
		int csum_flags = 0;
		uint16_t csum_data = 0;

 		KASSERT(pktlen < MCLBYTES);

		/* Check if RX TCP/UDP checksumming is being offloaded */
		if (sc->sc_coe_ctrl & SMSC_COE_CTRL_RX_EN) {
			DPRINTF("RX checksum offload checking", 0, 0, 0, 0);
			struct ether_header *eh = (struct ether_header *)pktbuf;
			const size_t cssz = sizeof(csum_data);

			/* Remove the extra 2 bytes of the csum */
			buflen -= cssz;

			/*
			 * The checksum appears to be simplistically calculated
			 * over the udp/tcp header and data up to the end of the
			 * eth frame.  Which means if the eth frame is padded
			 * the csum calculation is incorrectly performed over
			 * the padding bytes as well. Therefore to be safe we
			 * ignore the H/W csum on frames less than or equal to
			 * 64 bytes.
			 *
			 * Ignore H/W csum for non-IPv4 packets.
			 */
			DPRINTF("Ethertype %02jx pktlen %02jx",
			    be16toh(eh->ether_type), pktlen, 0, 0);
			if (be16toh(eh->ether_type) == ETHERTYPE_IP &&
			    pktlen > ETHER_MIN_LEN) {

				csum_flags |=
				    (M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_DATA);

				/*
				 * Copy the TCP/UDP checksum from the last 2
				 * bytes of the transfer and put in the
				 * csum_data field.
				 */
				memcpy(&csum_data, buf + pktlen - cssz, cssz);

				/*
				 * The data is copied in network order, but the
				 * csum algorithm in the kernel expects it to be
				 * in host network order.
				 */
				csum_data = ntohs(csum_data);
				DPRINTF("RX checksum offloaded (0x%04jx)",
				    csum_data, 0, 0, 0);
			}
		}

		/* round up to next longword */
		pktlen = (pktlen + 3) & ~0x3;

		/* total_len does not include the padding */
		if (pktlen > total_len)
			pktlen = total_len;

		buf += pktlen;
		total_len -= pktlen;

		/* push the packet up */
		usbnet_enqueue(un, pktbuf, buflen, csum_flags, csum_data,
		    mbuf_flags);

		count++;
	}

	if (count != 0)
		rnd_add_uint32(usbnet_rndsrc(un), count);
}

static unsigned
smsc_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	uint32_t txhdr;
	uint32_t frm_len = 0;

	const size_t hdrsz = sizeof(txhdr) * 2;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - hdrsz)
		return 0;

	/*
	 * Each frame is prefixed with two 32-bit values describing the
	 * length of the packet and buffer.
	 */
	txhdr = SMSC_TX_CTRL_0_BUF_SIZE(m->m_pkthdr.len) |
	    SMSC_TX_CTRL_0_FIRST_SEG | SMSC_TX_CTRL_0_LAST_SEG;
	txhdr = htole32(txhdr);
	memcpy(c->unc_buf, &txhdr, sizeof(txhdr));

	txhdr = SMSC_TX_CTRL_1_PKT_LENGTH(m->m_pkthdr.len);
	txhdr = htole32(txhdr);
	memcpy(c->unc_buf + sizeof(txhdr), &txhdr, sizeof(txhdr));

	frm_len += hdrsz;

	/* Next copy in the actual packet */
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf + frm_len);
	frm_len += m->m_pkthdr.len;

	return frm_len;
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(smsc)
