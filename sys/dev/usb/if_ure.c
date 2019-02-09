/*	$NetBSD: if_ure.c,v 1.3 2019/02/09 07:50:47 rin Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: if_ure.c,v 1.3 2019/02/09 07:50:47 rin Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <netinet/in.h>

#include <netinet/in_offload.h>		/* XXX for in_undefer_cksum() */
#ifdef INET6
#include <netinet6/in6_offload.h>	/* XXX for in6_undefer_cksum() */
#endif

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <dev/ic/rtl81x9reg.h>		/* XXX for RTK_GMEDIASTAT */
#include <dev/usb/if_urereg.h>
#include <dev/usb/if_urevar.h>

#define URE_PRINTF(sc, fmt, args...) \
	device_printf((sc)->ure_dev, "%s: " fmt, __func__, ##args);

#define URE_DEBUG
#ifdef URE_DEBUG
#define DPRINTF(x)	do { if (uredebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (uredebug >= (n)) printf x; } while (0)
int	uredebug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

static const struct usb_devno ure_devs[] = {
	{ USB_VENDOR_REALTEK, USB_PRODUCT_REALTEK_RTL8152 },
	{ USB_VENDOR_REALTEK, USB_PRODUCT_REALTEK_RTL8153 }
};

static int	ure_match(device_t, cfdata_t, void *);
static void	ure_attach(device_t, device_t, void *);
static int	ure_detach(device_t, int);
static int	ure_activate(device_t, enum devact);

static int	ure_ctl(struct ure_softc *, uint8_t, uint16_t, uint16_t,
		    void *, int);
static int	ure_read_mem(struct ure_softc *, uint16_t, uint16_t, void *,
		    int);
static int	ure_write_mem(struct ure_softc *, uint16_t, uint16_t, void *,
		    int);
static uint8_t	ure_read_1(struct ure_softc *, uint16_t, uint16_t);
static uint16_t	ure_read_2(struct ure_softc *, uint16_t, uint16_t);
static uint32_t ure_read_4(struct ure_softc *, uint16_t, uint16_t);
static int	ure_write_1(struct ure_softc *, uint16_t, uint16_t, uint32_t);
static int	ure_write_2(struct ure_softc *, uint16_t, uint16_t, uint32_t);
static int	ure_write_4(struct ure_softc *, uint16_t, uint16_t, uint32_t);
static uint16_t	ure_ocp_reg_read(struct ure_softc *, uint16_t);
static void	ure_ocp_reg_write(struct ure_softc *, uint16_t, uint16_t);

static int	ure_init(struct ifnet *);
static void	ure_stop(struct ifnet *, int);
static void	ure_start(struct ifnet *);
static void	ure_reset(struct ure_softc *);
static void	ure_miibus_statchg(struct ifnet *);
static int	ure_miibus_readreg(device_t, int, int, uint16_t *);
static int	ure_miibus_writereg(device_t, int, int, uint16_t);
static void	ure_lock_mii(struct ure_softc *);
static void	ure_unlock_mii(struct ure_softc *);

static int	ure_encap(struct ure_softc *, struct mbuf *, int);
static uint32_t	ure_txcsum(struct mbuf *);
static void	ure_rxeof(struct usbd_xfer *, void *, usbd_status);
static int	ure_rxcsum(struct ifnet *, struct ure_rxpkt *);
static void	ure_txeof(struct usbd_xfer *, void *, usbd_status);
static int	ure_rx_list_init(struct ure_softc *);
static int	ure_tx_list_init(struct ure_softc *);

static void	ure_tick_task(void *);
static void	ure_tick(void *);

static int	ure_ifmedia_upd(struct ifnet *);
static void	ure_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static int	ure_ioctl(struct ifnet *, u_long, void *);
static void	ure_rtl8152_init(struct ure_softc *);
static void	ure_rtl8153_init(struct ure_softc *);
static void	ure_disable_teredo(struct ure_softc *);
static void	ure_init_fifo(struct ure_softc *);

CFATTACH_DECL_NEW(ure, sizeof(struct ure_softc), ure_match, ure_attach,
    ure_detach, ure_activate);

static int
ure_ctl(struct ure_softc *sc, uint8_t rw, uint16_t val, uint16_t index,
    void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	if (sc->ure_dying)
		return 0;

	if (rw == URE_CTL_WRITE)
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = UR_SET_ADDRESS;
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, len);

	DPRINTFN(5, ("ure_ctl: rw %d, val 0x%04hu, index 0x%04hu, len %d\n",
	    rw, val, index, len));
	err = usbd_do_request(sc->ure_udev, &req, buf);
	if (err) {
		DPRINTF(("ure_ctl: error %d\n", err));
		return -1;
	}

	return 0;
}

static int
ure_read_mem(struct ure_softc *sc, uint16_t addr, uint16_t index,
    void *buf, int len)
{

	return ure_ctl(sc, URE_CTL_READ, addr, index, buf, len);
}

static int
ure_write_mem(struct ure_softc *sc, uint16_t addr, uint16_t index,
    void *buf, int len)
{

	return ure_ctl(sc, URE_CTL_WRITE, addr, index, buf, len);
}

static uint8_t
ure_read_1(struct ure_softc *sc, uint16_t reg, uint16_t index)
{
	uint32_t val;
	uint8_t temp[4];
	uint8_t shift;

	shift = (reg & 3) << 3;
	reg &= ~3;
	
	ure_read_mem(sc, reg, index, &temp, 4);
	val = UGETDW(temp);
	val >>= shift;

	return val & 0xff;
}

static uint16_t
ure_read_2(struct ure_softc *sc, uint16_t reg, uint16_t index)
{
	uint32_t val;
	uint8_t temp[4];
	uint8_t shift;

	shift = (reg & 2) << 3;
	reg &= ~3;

	ure_read_mem(sc, reg, index, &temp, 4);
	val = UGETDW(temp);
	val >>= shift;

	return val & 0xffff;
}

static uint32_t
ure_read_4(struct ure_softc *sc, uint16_t reg, uint16_t index)
{
	uint8_t temp[4];

	ure_read_mem(sc, reg, index, &temp, 4);
	return UGETDW(temp);
}

static int
ure_write_1(struct ure_softc *sc, uint16_t reg, uint16_t index, uint32_t val)
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
	return ure_write_mem(sc, reg, index | byen, &temp, 4);
}

static int
ure_write_2(struct ure_softc *sc, uint16_t reg, uint16_t index, uint32_t val)
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
	return ure_write_mem(sc, reg, index | byen, &temp, 4);
}

static int
ure_write_4(struct ure_softc *sc, uint16_t reg, uint16_t index, uint32_t val)
{
	uint8_t temp[4];

	USETDW(temp, val);
	return ure_write_mem(sc, reg, index | URE_BYTE_EN_DWORD, &temp, 4);
}

static uint16_t
ure_ocp_reg_read(struct ure_softc *sc, uint16_t addr)
{
	uint16_t reg;

	ure_write_2(sc, URE_PLA_OCP_GPHY_BASE, URE_MCU_TYPE_PLA, addr & 0xf000);
	reg = (addr & 0x0fff) | 0xb000;

	return ure_read_2(sc, reg, URE_MCU_TYPE_PLA);
}

static void
ure_ocp_reg_write(struct ure_softc *sc, uint16_t addr, uint16_t data)
{
	uint16_t reg;

	ure_write_2(sc, URE_PLA_OCP_GPHY_BASE, URE_MCU_TYPE_PLA, addr & 0xf000);
	reg = (addr & 0x0fff) | 0xb000;

	ure_write_2(sc, reg, URE_MCU_TYPE_PLA, data);
}

static int
ure_miibus_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	struct ure_softc *sc = device_private(dev);

	if (sc->ure_dying || sc->ure_phyno != phy) /* XXX */
		return -1;

	/* Let the rgephy driver read the URE_PLA_PHYSTATUS register. */
	if (reg == RTK_GMEDIASTAT) {
		*val = ure_read_1(sc, URE_PLA_PHYSTATUS, URE_MCU_TYPE_PLA);
		return 0;
	}

	ure_lock_mii(sc);
	*val = ure_ocp_reg_read(sc, URE_OCP_BASE_MII + reg * 2);
	ure_unlock_mii(sc);

	return 0;
}

static int
ure_miibus_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	struct ure_softc *sc = device_private(dev);

	if (sc->ure_dying || sc->ure_phyno != phy) /* XXX */
		return -1;

	ure_lock_mii(sc);
	ure_ocp_reg_write(sc, URE_OCP_BASE_MII + reg * 2, val);
	ure_unlock_mii(sc);

	return 0;
}

static void
ure_miibus_statchg(struct ifnet *ifp)
{
	struct ure_softc *sc;
	struct mii_data *mii;

	if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0)
		return;

	sc = ifp->if_softc;
	mii = GET_MII(sc);

	if (mii == NULL)
		return;

	sc->ure_flags &= ~URE_FLAG_LINK;
	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
			sc->ure_flags |= URE_FLAG_LINK;
			break;
		case IFM_1000_T:
			if ((sc->ure_flags & URE_FLAG_8152) != 0)
				break;
			sc->ure_flags |= URE_FLAG_LINK;
			break;
		default:
			break;
		}
	}
}

static int
ure_ifmedia_upd(struct ifnet *ifp)
{
	struct ure_softc *sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);
	int err;

	sc->ure_flags &= ~URE_FLAG_LINK;
	if (mii->mii_instance) {
		struct mii_softc *miisc;
		LIST_FOREACH(miisc, &mii->mii_phys, mii_list)
			mii_phy_reset(miisc);
	}

	err = mii_mediachg(mii);
	if (err == ENXIO)
		return 0;	/* XXX */
	else
		return err;
}

static void
ure_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct ure_softc *sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);

	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

static void
ure_iff(struct ure_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t hashes[2] = { 0, 0 };
	uint32_t hash;
	uint32_t rxmode;

	if (sc->ure_dying)
		return;

	rxmode = ure_read_4(sc, URE_PLA_RCR, URE_MCU_TYPE_PLA);
	rxmode &= ~URE_RCR_ACPT_ALL;
	ifp->if_flags &= ~IFF_ALLMULTI;

	/*
	 * Always accept frames destined to our station address.
	 * Always accept broadcast frames.
	 */
	rxmode |= URE_RCR_APM | URE_RCR_AB;

	if (ifp->if_flags & IFF_PROMISC) {
		rxmode |= URE_RCR_AAP;
allmulti:	ifp->if_flags |= IFF_ALLMULTI;
		rxmode |= URE_RCR_AM;
		hashes[0] = hashes[1] = 0xffffffff;
	} else {
		rxmode |= URE_RCR_AM;

		ETHER_FIRST_MULTI(step, &sc->ure_ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN))
				goto allmulti;

			hash = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN)
			    >> 26;
			if (hash < 32)
				hashes[0] |= (1 << hash);
			else
				hashes[1] |= (1 << (hash - 32));

			ETHER_NEXT_MULTI(step, enm);
		}

		hash = bswap32(hashes[0]);
		hashes[0] = bswap32(hashes[1]);
		hashes[1] = hash;
	}

	ure_write_4(sc, URE_PLA_MAR0, URE_MCU_TYPE_PLA, hashes[0]);
	ure_write_4(sc, URE_PLA_MAR4, URE_MCU_TYPE_PLA, hashes[1]);
	ure_write_4(sc, URE_PLA_RCR, URE_MCU_TYPE_PLA, rxmode);
}

static void
ure_reset(struct ure_softc *sc)
{
	int i;

	ure_write_1(sc, URE_PLA_CR, URE_MCU_TYPE_PLA, URE_CR_RST);

	for (i = 0; i < URE_TIMEOUT; i++) {
		if (!(ure_read_1(sc, URE_PLA_CR, URE_MCU_TYPE_PLA) &
		    URE_CR_RST))
			break;
		usbd_delay_ms(sc->ure_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(sc, "reset never completed\n");
}

static int
ure_init(struct ifnet *ifp)
{
	struct ure_softc *sc = ifp->if_softc;
	struct ure_chain *c;
	usbd_status err;
	int s, i;
	uint8_t eaddr[8];

	s = splnet();

	/* Cancel pending I/O. */
	if (ifp->if_flags & IFF_RUNNING)
		ure_stop(ifp, 1);

	/* Set MAC address. */
	memset(eaddr, 0, sizeof(eaddr));
	memcpy(eaddr, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
	ure_write_1(sc, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_CONFIG);
	ure_write_mem(sc, URE_PLA_IDR, URE_MCU_TYPE_PLA | URE_BYTE_EN_SIX_BYTES,
	    eaddr, 8);
	ure_write_1(sc, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML);

	/* Reset the packet filter. */
	ure_write_2(sc, URE_PLA_FMC, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_FMC, URE_MCU_TYPE_PLA) &
	    ~URE_FMC_FCR_MCU_EN);
	ure_write_2(sc, URE_PLA_FMC, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_FMC, URE_MCU_TYPE_PLA) |
	    URE_FMC_FCR_MCU_EN);
	    
	/* Enable transmit and receive. */
	ure_write_1(sc, URE_PLA_CR, URE_MCU_TYPE_PLA,
	    ure_read_1(sc, URE_PLA_CR, URE_MCU_TYPE_PLA) | URE_CR_RE |
	    URE_CR_TE);

	ure_write_2(sc, URE_PLA_MISC_1, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_MISC_1, URE_MCU_TYPE_PLA) &
	    ~URE_RXDY_GATED_EN);

	/* Load the multicast filter. */
	ure_iff(sc);

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->ure_iface, sc->ure_ed[URE_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->ure_ep[URE_ENDPT_RX]);
	if (err) {
		URE_PRINTF(sc, "open rx pipe failed: %s\n", usbd_errstr(err));
		splx(s);
		return EIO;
	}

	err = usbd_open_pipe(sc->ure_iface, sc->ure_ed[URE_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->ure_ep[URE_ENDPT_TX]);
	if (err) {
		URE_PRINTF(sc, "open tx pipe failed: %s\n", usbd_errstr(err));
		splx(s);
		return EIO;
	}

	if (ure_rx_list_init(sc)) {
		URE_PRINTF(sc, "rx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	if (ure_tx_list_init(sc)) {
		URE_PRINTF(sc, "tx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	/* Start up the receive pipe. */
	for (i = 0; i < URE_RX_LIST_CNT; i++) {
		c = &sc->ure_cdata.rx_chain[i];
		usbd_setup_xfer(c->uc_xfer, c, c->uc_buf, sc->ure_bufsz,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, ure_rxeof);
		usbd_transfer(c->uc_xfer);
	}

	/* Indicate we are up and running. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	callout_reset(&sc->ure_stat_ch, hz, ure_tick, sc);

	return 0;
}

static void
ure_start(struct ifnet *ifp)
{
	struct ure_softc *sc = ifp->if_softc;
	struct mbuf *m;
	struct ure_cdata *cd = &sc->ure_cdata;
	int idx;

	if ((sc->ure_flags & URE_FLAG_LINK) == 0 ||
	    (ifp->if_flags & (IFF_OACTIVE|IFF_RUNNING)) != IFF_RUNNING) {
		return;
	}

	idx = cd->tx_prod;
	while (cd->tx_cnt < URE_TX_LIST_CNT) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (ure_encap(sc, m, idx)) {
			ifp->if_oerrors++;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);

		bpf_mtap(ifp, m, BPF_D_OUT);
		m_freem(m);

		idx = (idx + 1) % URE_TX_LIST_CNT;
		cd->tx_cnt++;
	}
	cd->tx_prod = idx;

	if (cd->tx_cnt >= URE_TX_LIST_CNT)
		ifp->if_flags |= IFF_OACTIVE;
}

static void
ure_tick(void *xsc)
{
	struct ure_softc *sc = xsc;

	if (sc == NULL)
		return;

	if (sc->ure_dying)
		return;

	usb_add_task(sc->ure_udev, &sc->ure_tick_task, USB_TASKQ_DRIVER);
}

static void
ure_stop(struct ifnet *ifp, int disable __unused)
{
	struct ure_softc *sc = ifp->if_softc;
	struct ure_chain *c;
	usbd_status err;
	int i;

	ure_reset(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	callout_stop(&sc->ure_stat_ch);

	sc->ure_flags &= ~URE_FLAG_LINK; /* XXX */

	if (sc->ure_ep[URE_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->ure_ep[URE_ENDPT_RX]);
		if (err)
			URE_PRINTF(sc, "abort rx pipe failed: %s\n",
			    usbd_errstr(err));
	}

	if (sc->ure_ep[URE_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->ure_ep[URE_ENDPT_TX]);
		if (err)
			URE_PRINTF(sc, "abort tx pipe failed: %s\n",
			    usbd_errstr(err));
	}

	for (i = 0; i < URE_RX_LIST_CNT; i++) {
		c = &sc->ure_cdata.rx_chain[i];
		if (c->uc_xfer != NULL) {
			usbd_destroy_xfer(c->uc_xfer);
			c->uc_xfer = NULL;
		}
	}

	for (i = 0; i < URE_TX_LIST_CNT; i++) {
		c = &sc->ure_cdata.tx_chain[i];
		if (c->uc_xfer != NULL) {
			usbd_destroy_xfer(c->uc_xfer);
			c->uc_xfer = NULL;
		}
	}

	if (sc->ure_ep[URE_ENDPT_RX] != NULL) {
		err = usbd_close_pipe(sc->ure_ep[URE_ENDPT_RX]);
		if (err)
			URE_PRINTF(sc, "close rx pipe failed: %s\n",
			    usbd_errstr(err));
		sc->ure_ep[URE_ENDPT_RX] = NULL;
	}

	if (sc->ure_ep[URE_ENDPT_TX] != NULL) {
		err = usbd_close_pipe(sc->ure_ep[URE_ENDPT_TX]);
		if (err)
			URE_PRINTF(sc, "close tx pipe failed: %s\n",
			    usbd_errstr(err));
		sc->ure_ep[URE_ENDPT_TX] = NULL;
	}
}

static void
ure_rtl8152_init(struct ure_softc *sc)
{
	uint32_t pwrctrl;

	/* Disable ALDPS. */
	ure_ocp_reg_write(sc, URE_OCP_ALDPS_CONFIG, URE_ENPDNPS | URE_LINKENA |
	    URE_DIS_SDSAVE);
	usbd_delay_ms(sc->ure_udev, 20);

	if (sc->ure_chip & URE_CHIP_VER_4C00) {
		ure_write_2(sc, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA,
		    ure_read_2(sc, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA) &
		    ~URE_LED_MODE_MASK);
	}

	ure_write_2(sc, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_POWER_CUT);
	ure_write_2(sc, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB) &
	    ~URE_RESUME_INDICATE);

	ure_write_2(sc, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA) |
	    URE_TX_10M_IDLE_EN | URE_PFM_PWM_SWITCH);
	pwrctrl = ure_read_4(sc, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA);
	pwrctrl &= ~URE_MCU_CLK_RATIO_MASK;
	pwrctrl |= URE_MCU_CLK_RATIO | URE_D3_CLK_GATED_EN;
	ure_write_4(sc, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA, pwrctrl);
	ure_write_2(sc, URE_PLA_GPHY_INTR_IMR, URE_MCU_TYPE_PLA,
	    URE_GPHY_STS_MSK | URE_SPEED_DOWN_MSK | URE_SPDWN_RXDV_MSK |
	    URE_SPDWN_LINKCHG_MSK);

	/* Enable Rx aggregation. */
	ure_write_2(sc, URE_USB_USB_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_USB_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_RX_AGG_DISABLE);

	/* Disable ALDPS. */
	ure_ocp_reg_write(sc, URE_OCP_ALDPS_CONFIG, URE_ENPDNPS | URE_LINKENA |
	    URE_DIS_SDSAVE);
	usbd_delay_ms(sc->ure_udev, 20);

	ure_init_fifo(sc);

	ure_write_1(sc, URE_USB_TX_AGG, URE_MCU_TYPE_USB,
	    URE_TX_AGG_MAX_THRESHOLD);
	ure_write_4(sc, URE_USB_RX_BUF_TH, URE_MCU_TYPE_USB, URE_RX_THR_HIGH);
	ure_write_4(sc, URE_USB_TX_DMA, URE_MCU_TYPE_USB,
	    URE_TEST_MODE_DISABLE | URE_TX_SIZE_ADJUST1);
}

static void
ure_rtl8153_init(struct ure_softc *sc)
{
	uint16_t val;
	uint8_t u1u2[8];
	int i;

	/* Disable ALDPS. */
	ure_ocp_reg_write(sc, URE_OCP_POWER_CFG,
	    ure_ocp_reg_read(sc, URE_OCP_POWER_CFG) & ~URE_EN_ALDPS);
	usbd_delay_ms(sc->ure_udev, 20);

	memset(u1u2, 0x00, sizeof(u1u2));
	ure_write_mem(sc, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

        for (i = 0; i < URE_TIMEOUT; i++) {
		if (ure_read_2(sc, URE_PLA_BOOT_CTRL, URE_MCU_TYPE_PLA) &
		    URE_AUTOLOAD_DONE)
			break;
		usbd_delay_ms(sc->ure_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(sc, "timeout waiting for chip autoload\n");

	for (i = 0; i < URE_TIMEOUT; i++) {
		val = ure_ocp_reg_read(sc, URE_OCP_PHY_STATUS) &
		    URE_PHY_STAT_MASK;
		if (val == URE_PHY_STAT_LAN_ON || val == URE_PHY_STAT_PWRDN)
			break;
		usbd_delay_ms(sc->ure_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(sc, "timeout waiting for phy to stabilize\n");
	
	ure_write_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_U2P3_ENABLE);

	if (sc->ure_chip & URE_CHIP_VER_5C10) {
		val = ure_read_2(sc, URE_USB_SSPHYLINK2, URE_MCU_TYPE_USB);
		val &= ~URE_PWD_DN_SCALE_MASK;
		val |= URE_PWD_DN_SCALE(96);
		ure_write_2(sc, URE_USB_SSPHYLINK2, URE_MCU_TYPE_USB, val);

		ure_write_1(sc, URE_USB_USB2PHY, URE_MCU_TYPE_USB,
		    ure_read_1(sc, URE_USB_USB2PHY, URE_MCU_TYPE_USB) |
		    URE_USB2PHY_L1 | URE_USB2PHY_SUSPEND);
	} else if (sc->ure_chip & URE_CHIP_VER_5C20) {
		ure_write_1(sc, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA,
		    ure_read_1(sc, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA) &
		    ~URE_ECM_ALDPS);
	}
	if (sc->ure_chip & (URE_CHIP_VER_5C20 | URE_CHIP_VER_5C30)) {
		val = ure_read_1(sc, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB);
		if (ure_read_2(sc, URE_USB_BURST_SIZE, URE_MCU_TYPE_USB) ==
		    0)
			val &= ~URE_DYNAMIC_BURST;
		else
			val |= URE_DYNAMIC_BURST;
		ure_write_1(sc, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB, val);
	}

	ure_write_1(sc, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB,
	    ure_read_1(sc, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB) |
	    URE_EP4_FULL_FC);
	
	ure_write_2(sc, URE_USB_WDT11_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_WDT11_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_TIMER11_EN);

	ure_write_2(sc, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA) &
	    ~URE_LED_MODE_MASK);
	    
	if ((sc->ure_chip & URE_CHIP_VER_5C10) &&
	    sc->ure_udev->ud_speed != USB_SPEED_SUPER)
		val = URE_LPM_TIMER_500MS;
	else
		val = URE_LPM_TIMER_500US;
	ure_write_1(sc, URE_USB_LPM_CTRL, URE_MCU_TYPE_USB,
	    val | URE_FIFO_EMPTY_1FB | URE_ROK_EXIT_LPM);

	val = ure_read_2(sc, URE_USB_AFE_CTRL2, URE_MCU_TYPE_USB);
	val &= ~URE_SEN_VAL_MASK;
	val |= URE_SEN_VAL_NORMAL | URE_SEL_RXIDLE;
	ure_write_2(sc, URE_USB_AFE_CTRL2, URE_MCU_TYPE_USB, val);

	ure_write_2(sc, URE_USB_CONNECT_TIMER, URE_MCU_TYPE_USB, 0x0001);

	ure_write_2(sc, URE_USB_POWER_CUT, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_POWER_CUT, URE_MCU_TYPE_USB) &
	    ~(URE_PWR_EN | URE_PHASE2_EN));
	ure_write_2(sc, URE_USB_MISC_0, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_MISC_0, URE_MCU_TYPE_USB) &
	    ~URE_PCUT_STATUS);

	memset(u1u2, 0xff, sizeof(u1u2));
	ure_write_mem(sc, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

	ure_write_2(sc, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA,
	    URE_ALDPS_SPDWN_RATIO);
	ure_write_2(sc, URE_PLA_MAC_PWR_CTRL2, URE_MCU_TYPE_PLA,
	    URE_EEE_SPDWN_RATIO);
	ure_write_2(sc, URE_PLA_MAC_PWR_CTRL3, URE_MCU_TYPE_PLA,
	    URE_PKT_AVAIL_SPDWN_EN | URE_SUSPEND_SPDWN_EN |
	    URE_U1U2_SPDWN_EN | URE_L1_SPDWN_EN);
	ure_write_2(sc, URE_PLA_MAC_PWR_CTRL4, URE_MCU_TYPE_PLA,
	    URE_PWRSAVE_SPDWN_EN | URE_RXDV_SPDWN_EN | URE_TX10MIDLE_EN |
	    URE_TP100_SPDWN_EN | URE_TP500_SPDWN_EN | URE_TP1000_SPDWN_EN |
	    URE_EEE_SPDWN_EN);

	val = ure_read_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB);
	if (!(sc->ure_chip & (URE_CHIP_VER_5C00 | URE_CHIP_VER_5C10)))
		val |= URE_U2P3_ENABLE;
	else
		val &= ~URE_U2P3_ENABLE;
	ure_write_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB, val);

	memset(u1u2, 0x00, sizeof(u1u2));
        ure_write_mem(sc, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

	/* Disable ALDPS. */
	ure_ocp_reg_write(sc, URE_OCP_POWER_CFG,
	    ure_ocp_reg_read(sc, URE_OCP_POWER_CFG) & ~URE_EN_ALDPS);
	usbd_delay_ms(sc->ure_udev, 20);

	ure_init_fifo(sc);

	/* Enable Rx aggregation. */
	ure_write_2(sc, URE_USB_USB_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(sc, URE_USB_USB_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_RX_AGG_DISABLE);

	val = ure_read_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB);
	if (!(sc->ure_chip & (URE_CHIP_VER_5C00 | URE_CHIP_VER_5C10)))
		val |= URE_U2P3_ENABLE;
	else
		val &= ~URE_U2P3_ENABLE;
	ure_write_2(sc, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB, val);

	memset(u1u2, 0xff, sizeof(u1u2));
	ure_write_mem(sc, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));
}

static void
ure_disable_teredo(struct ure_softc *sc)
{

	ure_write_4(sc, URE_PLA_TEREDO_CFG, URE_MCU_TYPE_PLA,
	    ure_read_4(sc, URE_PLA_TEREDO_CFG, URE_MCU_TYPE_PLA) & 
	    ~(URE_TEREDO_SEL | URE_TEREDO_RS_EVENT_MASK | URE_OOB_TEREDO_EN));
	ure_write_2(sc, URE_PLA_WDT6_CTRL, URE_MCU_TYPE_PLA,
	    URE_WDT6_SET_MODE);
	ure_write_2(sc, URE_PLA_REALWOW_TIMER, URE_MCU_TYPE_PLA, 0);
	ure_write_4(sc, URE_PLA_TEREDO_TIMER, URE_MCU_TYPE_PLA, 0);
}

static void
ure_init_fifo(struct ure_softc *sc)
{
	uint32_t rx_fifo1, rx_fifo2;
	int i;

	ure_write_2(sc, URE_PLA_MISC_1, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_MISC_1, URE_MCU_TYPE_PLA) |
	    URE_RXDY_GATED_EN);

	ure_disable_teredo(sc);

	ure_write_4(sc, URE_PLA_RCR, URE_MCU_TYPE_PLA,
	    ure_read_4(sc, URE_PLA_RCR, URE_MCU_TYPE_PLA) &
	    ~URE_RCR_ACPT_ALL);

	if (!(sc->ure_flags & URE_FLAG_8152)) {
		if (sc->ure_chip & (URE_CHIP_VER_5C00 | URE_CHIP_VER_5C10 |
		    URE_CHIP_VER_5C20))
			ure_ocp_reg_write(sc, URE_OCP_ADC_CFG,
			    URE_CKADSEL_L | URE_ADC_EN | URE_EN_EMI_L);
		if (sc->ure_chip & URE_CHIP_VER_5C00)
			ure_ocp_reg_write(sc, URE_OCP_EEE_CFG,
			    ure_ocp_reg_read(sc, URE_OCP_EEE_CFG) & 
			    ~URE_CTAP_SHORT_EN);
		ure_ocp_reg_write(sc, URE_OCP_POWER_CFG,
		    ure_ocp_reg_read(sc, URE_OCP_POWER_CFG) |
		    URE_EEE_CLKDIV_EN);
		ure_ocp_reg_write(sc, URE_OCP_DOWN_SPEED,
		    ure_ocp_reg_read(sc, URE_OCP_DOWN_SPEED) |
		    URE_EN_10M_BGOFF);
		ure_ocp_reg_write(sc, URE_OCP_POWER_CFG,
		    ure_ocp_reg_read(sc, URE_OCP_POWER_CFG) |
		    URE_EN_10M_PLLOFF);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_ADDR, URE_SRAM_IMPEDANCE);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_DATA, 0x0b13);
		ure_write_2(sc, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA,
		    ure_read_2(sc, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA) |
		    URE_PFM_PWM_SWITCH);

		/* Enable LPF corner auto tune. */
		ure_ocp_reg_write(sc, URE_OCP_SRAM_ADDR, URE_SRAM_LPF_CFG);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_DATA, 0xf70f);

		/* Adjust 10M amplitude. */
		ure_ocp_reg_write(sc, URE_OCP_SRAM_ADDR, URE_SRAM_10M_AMP1);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_DATA, 0x00af);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_ADDR, URE_SRAM_10M_AMP2);
		ure_ocp_reg_write(sc, URE_OCP_SRAM_DATA, 0x0208);
	}

	ure_reset(sc);

	ure_write_1(sc, URE_PLA_CR, URE_MCU_TYPE_PLA, 0);

	ure_write_1(sc, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA,
	    ure_read_1(sc, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
	    ~URE_NOW_IS_OOB);

	ure_write_2(sc, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA) &
	    ~URE_MCU_BORW_EN);
	for (i = 0; i < URE_TIMEOUT; i++) {
		if (ure_read_1(sc, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
		    URE_LINK_LIST_READY)
			break;
		usbd_delay_ms(sc->ure_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(sc, "timeout waiting for OOB control\n");
	ure_write_2(sc, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA) |
	    URE_RE_INIT_LL);
	for (i = 0; i < URE_TIMEOUT; i++) {
		if (ure_read_1(sc, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
		    URE_LINK_LIST_READY)
			break;
		usbd_delay_ms(sc->ure_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(sc, "timeout waiting for OOB control\n");

	ure_write_2(sc, URE_PLA_CPCR, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_CPCR, URE_MCU_TYPE_PLA) &
	    ~URE_CPCR_RX_VLAN);
	ure_write_2(sc, URE_PLA_TCR0, URE_MCU_TYPE_PLA,
	    ure_read_2(sc, URE_PLA_TCR0, URE_MCU_TYPE_PLA) |
	    URE_TCR0_AUTO_FIFO);

	/* Configure Rx FIFO threshold and coalescing. */
	ure_write_4(sc, URE_PLA_RXFIFO_CTRL0, URE_MCU_TYPE_PLA,
	    URE_RXFIFO_THR1_NORMAL);
	if (sc->ure_udev->ud_speed == USB_SPEED_FULL) {
		rx_fifo1 = URE_RXFIFO_THR2_FULL;
		rx_fifo2 = URE_RXFIFO_THR3_FULL;
	} else {
		rx_fifo1 = URE_RXFIFO_THR2_HIGH;
		rx_fifo2 = URE_RXFIFO_THR3_HIGH;
	}
	ure_write_4(sc, URE_PLA_RXFIFO_CTRL1, URE_MCU_TYPE_PLA, rx_fifo1);
	ure_write_4(sc, URE_PLA_RXFIFO_CTRL2, URE_MCU_TYPE_PLA, rx_fifo2);

	/* Configure Tx FIFO threshold. */
	ure_write_4(sc, URE_PLA_TXFIFO_CTRL, URE_MCU_TYPE_PLA,
	    URE_TXFIFO_THR_NORMAL);
}

int
ure_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ure_softc *sc = ifp->if_softc;
	int s, error = 0, oflags = ifp->if_flags;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			ure_stop(ifp, 1);
			break;
		case IFF_UP:
			ure_init(ifp);
			break;
		case IFF_UP | IFF_RUNNING:
			if ((ifp->if_flags ^ oflags) == IFF_PROMISC)
				ure_iff(sc);
			else
				ure_init(ifp);
		}
		break;
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;
		error = 0;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			break;
		switch (cmd) {
		case SIOCADDMULTI:
		case SIOCDELMULTI:
			ure_iff(sc);
			break;
		default:
			break;
		}
	}

	splx(s);

	return error;
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
	struct ure_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct ifnet *ifp;
	struct mii_data *mii;
	int error, i, s;
	uint16_t ver;
	uint8_t eaddr[8]; /* 2byte padded */
	char *devinfop;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->ure_dev = self;
	sc->ure_udev = dev;

	devinfop = usbd_devinfo_alloc(sc->ure_udev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

#define URE_CONFIG_NO	1 /* XXX */
	error = usbd_set_config_no(dev, URE_CONFIG_NO, 1);
	if (error) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
		    usbd_errstr(error));
		return; /* XXX */
	}

	if (uaa->uaa_product == USB_PRODUCT_REALTEK_RTL8152)
		sc->ure_flags |= URE_FLAG_8152;

	usb_init_task(&sc->ure_tick_task, ure_tick_task, sc, 0);
	mutex_init(&sc->ure_mii_lock, MUTEX_DEFAULT, IPL_NONE);

#define URE_IFACE_IDX  0 /* XXX */
	error = usbd_device2interface_handle(dev, URE_IFACE_IDX, &sc->ure_iface);
	if (error) {
		aprint_error_dev(self, "failed to get interface handle: %s\n",
		    usbd_errstr(error));
		return; /* XXX */
	}

	sc->ure_bufsz = 16 * 1024;

	id = usbd_get_interface_descriptor(sc->ure_iface);
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->ure_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return; /* XXX */
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->ure_ed[URE_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->ure_ed[URE_ENDPT_TX] = ed->bEndpointAddress;
		}
	}

	s = splnet();

	sc->ure_phyno = 0;

	ver = ure_read_2(sc, URE_PLA_TCR1, URE_MCU_TYPE_PLA) & URE_VERSION_MASK;
	switch (ver) {
	case 0x4c00:
		sc->ure_chip |= URE_CHIP_VER_4C00;
		break;
	case 0x4c10:
		sc->ure_chip |= URE_CHIP_VER_4C10;
		break;
	case 0x5c00:
		sc->ure_chip |= URE_CHIP_VER_5C00;
		break;
	case 0x5c10:
		sc->ure_chip |= URE_CHIP_VER_5C10;
		break;
	case 0x5c20:
		sc->ure_chip |= URE_CHIP_VER_5C20;
		break;
	case 0x5c30:
		sc->ure_chip |= URE_CHIP_VER_5C30;
		break;
	default:
		/* fake addr?  or just fail? */
		break;
	}
	aprint_normal_dev(self, "RTL%d %sver %04x\n",
	    (sc->ure_flags & URE_FLAG_8152) ? 8152 : 8153,
	    (sc->ure_chip != 0) ? "" : "unknown ",
	    ver);

	if (sc->ure_flags & URE_FLAG_8152)
		ure_rtl8152_init(sc);
	else
		ure_rtl8153_init(sc);

	if (sc->ure_chip & URE_CHIP_VER_4C00)
		ure_read_mem(sc, URE_PLA_IDR, URE_MCU_TYPE_PLA, eaddr,
		    sizeof(eaddr));
	else
		ure_read_mem(sc, URE_PLA_BACKUP, URE_MCU_TYPE_PLA, eaddr,
		    sizeof(eaddr));

	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(eaddr));

	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->ure_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = ure_init;
	ifp->if_ioctl = ure_ioctl;
	ifp->if_start = ure_start;
	ifp->if_stop = ure_stop;

	/*
	 * We don't support TSOv4 and v6 for now, that are required to
	 * be handled in software for some cases.
	 */
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Tx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx;
#ifdef INET6
	ifp->if_capabilities |= IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_UDPv6_Tx;
#endif
	if (sc->ure_chip & ~URE_CHIP_VER_4C00) {
		ifp->if_capabilities |= IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
		    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;
	}
	sc->ure_ec.ec_capabilities = ETHERCAP_VLAN_MTU;
#ifdef notyet
	sc->ure_ec.ec_capabilities |= ETHERCAP_JUMBO_MTU;
#endif

	IFQ_SET_READY(&ifp->if_snd);

	mii = GET_MII(sc);
	mii->mii_ifp = ifp;
	mii->mii_readreg = ure_miibus_readreg;
	mii->mii_writereg = ure_miibus_writereg;
	mii->mii_statchg = ure_miibus_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;

	sc->ure_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ure_ifmedia_upd, ure_ifmedia_sts);
	mii_attach(self, mii, 0xffffffff, sc->ure_phyno, MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

	rnd_attach_source(&sc->ure_rnd_source, device_xname(sc->ure_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	callout_init(&sc->ure_stat_ch, 0);

	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->ure_udev, sc->ure_dev);
}

static int
ure_detach(device_t self, int flags)
{
	struct ure_softc *sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);
	int s;

	sc->ure_dying = true;

	callout_halt(&sc->ure_stat_ch, NULL);

	if (sc->ure_ep[URE_ENDPT_TX] != NULL)
		usbd_abort_pipe(sc->ure_ep[URE_ENDPT_TX]);
	if (sc->ure_ep[URE_ENDPT_RX] != NULL)
		usbd_abort_pipe(sc->ure_ep[URE_ENDPT_RX]);

	usb_rem_task_wait(sc->ure_udev, &sc->ure_tick_task, USB_TASKQ_DRIVER,
	    NULL);

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING)
		ure_stop(ifp, 1);

	callout_destroy(&sc->ure_stat_ch);
	rnd_detach_source(&sc->ure_rnd_source);
	mii_detach(&sc->ure_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->ure_mii.mii_media, IFM_INST_ANY);
	if (ifp->if_softc != NULL) {
		ether_ifdetach(ifp);
		if_detach(ifp);
	}

	if (--sc->ure_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_waitold(sc->ure_dev);
	}
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->ure_udev, sc->ure_dev);

	mutex_destroy(&sc->ure_mii_lock);

	return 0;
}

static int
ure_activate(device_t self, enum devact act)
{
	struct ure_softc *sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(ifp);
		sc->ure_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
	return 0;
}

static void
ure_tick_task(void *xsc)
{
	struct ure_softc *sc = xsc;
	struct ifnet *ifp = GET_IFP(sc);
	struct mii_data *mii;
	int s;

	if (sc == NULL)
		return;

	if (sc->ure_dying)
		return;

	mii = GET_MII(sc);

	s = splnet();
	mii_tick(mii);
	if ((sc->ure_flags & URE_FLAG_LINK) == 0)
		ure_miibus_statchg(ifp);
	callout_reset(&sc->ure_stat_ch, hz, ure_tick, sc);
	splx(s);
}

static void
ure_lock_mii(struct ure_softc *sc)
{

	sc->ure_refcnt++;
	mutex_enter(&sc->ure_mii_lock);
}

static void
ure_unlock_mii(struct ure_softc *sc)
{

	mutex_exit(&sc->ure_mii_lock);
	if (--sc->ure_refcnt < 0)
		usb_detach_wakeupold(sc->ure_dev);
}

static void
ure_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct ure_chain *c = (struct ure_chain *)priv;
	struct ure_softc *sc = c->uc_sc;
	struct ifnet *ifp = GET_IFP(sc);
	uint8_t *buf = c->uc_buf;
	uint32_t total_len;
	uint16_t pktlen = 0;
	struct mbuf *m;
	int s;
	struct ure_rxpkt rxhdr;
	
	if (sc->ure_dying)
		return;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_INVAL)
			return;	/* XXX plugged out or down */
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		if (usbd_ratecheck(&sc->ure_rx_notice))
			URE_PRINTF(sc, "usb errors on rx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(
			    sc->ure_ep[URE_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);
	DPRINTFN(3, ("received %d bytes\n", total_len));

	KASSERTMSG(total_len <= sc->ure_bufsz, "%u vs %u",
	    total_len, sc->ure_bufsz);

	do {
		if (total_len < sizeof(rxhdr)) {
			DPRINTF(("too few bytes left for a packet header\n"));
			ifp->if_ierrors++;
			goto done;
		}

		buf += roundup(pktlen, 8);

		memcpy(&rxhdr, buf, sizeof(rxhdr));
		total_len -= sizeof(rxhdr);

		pktlen = le32toh(rxhdr.ure_pktlen) & URE_RXPKT_LEN_MASK;
		DPRINTFN(4, ("next packet is %d bytes\n", pktlen));
		if (pktlen > total_len) {
			DPRINTF(("not enough bytes left for next packet\n"));
			ifp->if_ierrors++;
			goto done;
		}

		total_len -= roundup(pktlen, 8);
		buf += sizeof(rxhdr);

		m = m_devget(buf, pktlen - ETHER_CRC_LEN, 0, ifp);
		if (m == NULL) {
			DPRINTF(("unable to allocate mbuf for next packet\n"));
			ifp->if_ierrors++;
			goto done;
		}

		m->m_pkthdr.csum_flags = ure_rxcsum(ifp, &rxhdr);

		s = splnet();
		if_percpuq_enqueue(ifp->if_percpuq, m);
		splx(s);
	} while (total_len > 0);

done:
	usbd_setup_xfer(xfer, c, c->uc_buf, sc->ure_bufsz,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, ure_rxeof);
	usbd_transfer(xfer);
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

static void
ure_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct ure_chain *c = priv;
	struct ure_softc *sc = c->uc_sc;
	struct ure_cdata *cd = &sc->ure_cdata;
	struct ifnet *ifp = GET_IFP(sc);
	int s;

	if (sc->ure_dying)
		return;

	DPRINTFN(2, ("tx completion\n"));

	s = splnet();

	KASSERT(cd->tx_cnt > 0);
	cd->tx_cnt--;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		if (usbd_ratecheck(&sc->ure_tx_notice))
			URE_PRINTF(sc, "usb error on tx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(
			    sc->ure_ep[URE_ENDPT_TX]);
		splx(s);
		return;
	}

	ifp->if_flags &= ~IFF_OACTIVE;

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		ure_start(ifp);

	splx(s);
}

static int
ure_tx_list_init(struct ure_softc *sc)
{
	struct ure_cdata *cd;
	struct ure_chain *c;
	int i, error;

	cd = &sc->ure_cdata;
	for (i = 0; i < URE_TX_LIST_CNT; i++) {
		c = &cd->tx_chain[i];
		c->uc_sc = sc;
		if (c->uc_xfer == NULL) {
			error = usbd_create_xfer(sc->ure_ep[URE_ENDPT_TX],
			    sc->ure_bufsz, USBD_FORCE_SHORT_XFER, 0,
			    &c->uc_xfer);
			if (error)
				return error;
			c->uc_buf = usbd_get_buffer(c->uc_xfer);
		}
	}

	cd->tx_prod = cd->tx_cnt = 0;

	return 0;
}

static int
ure_rx_list_init(struct ure_softc *sc)
{
	struct ure_cdata *cd;
	struct ure_chain *c;
	int i, error;

	cd = &sc->ure_cdata;
	for (i = 0; i < URE_RX_LIST_CNT; i++) {
		c = &cd->rx_chain[i];
		c->uc_sc = sc;
		error = usbd_create_xfer(sc->ure_ep[URE_ENDPT_RX],
		    sc->ure_bufsz, 0, 0, &c->uc_xfer);
		if (error)
			return error;
		c->uc_buf = usbd_get_buffer(c->uc_xfer);
	}

	return 0;
}

static int
ure_encap(struct ure_softc *sc, struct mbuf *m, int idx)
{
	struct ifnet *ifp = GET_IFP(sc);
	struct ure_chain *c;
	usbd_status err;
	struct ure_txpkt txhdr;
	uint32_t frm_len = 0;
	uint8_t *buf;

	c = &sc->ure_cdata.tx_chain[idx];
	buf = c->uc_buf;

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

	if (__predict_false(c->uc_xfer == NULL))
		return EIO;	/* XXX plugged out or down */

	DPRINTFN(2, ("tx %d bytes\n", frm_len));
	usbd_setup_xfer(c->uc_xfer, c, c->uc_buf, frm_len,
	    USBD_FORCE_SHORT_XFER, 10000, ure_txeof);

	err = usbd_transfer(c->uc_xfer);
	if (err != USBD_IN_PROGRESS) {
		ure_stop(ifp, 0);
		return EIO;
	}

	return 0;
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
