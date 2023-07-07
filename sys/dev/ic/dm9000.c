/*	$NetBSD: dm9000.c,v 1.36 2023/07/07 07:22:18 martin Exp $	*/

/*
 * Copyright (c) 2009 Paul Fleischer
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* based on sys/dev/ic/cs89x0.c */
/*
 * Copyright (c) 2004 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/cprng.h>
#include <sys/rndsource.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <net/bpf.h>

#include <dev/ic/dm9000var.h>
#include <dev/ic/dm9000reg.h>

#if 1
#undef DM9000_DEBUG
#undef DM9000_TX_DEBUG
#undef DM9000_TX_DATA_DEBUG
#undef DM9000_RX_DEBUG
#undef  DM9000_RX_DATA_DEBUG
#else
#define DM9000_DEBUG
#define  DM9000_TX_DEBUG
#define DM9000_TX_DATA_DEBUG
#define DM9000_RX_DEBUG
#define  DM9000_RX_DATA_DEBUG
#endif

#ifdef DM9000_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_TX_DEBUG
#define TX_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define TX_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_RX_DEBUG
#define RX_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define RX_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_RX_DATA_DEBUG
#define RX_DATA_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define RX_DATA_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_TX_DATA_DEBUG
#define TX_DATA_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define TX_DATA_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

static void dme_reset(struct dme_softc *);
static int dme_init(struct ifnet *);
static void dme_stop(struct ifnet *, int);
static void dme_start(struct ifnet *);
static int dme_ioctl(struct ifnet *, u_long, void *);

static void dme_set_rcvfilt(struct dme_softc *);
static void mii_statchg(struct ifnet *);
static void lnkchg(struct dme_softc *);
static void phy_tick(void *);
static int mii_readreg(device_t, int, int, uint16_t *);
static int mii_writereg(device_t, int, int, uint16_t);

static void dme_prepare(struct ifnet *);
static void dme_transmit(struct ifnet *);
static void dme_receive(struct ifnet *);

static int pkt_read_2(struct dme_softc *, struct mbuf **);
static int pkt_write_2(struct dme_softc *, struct mbuf *);
static int pkt_read_1(struct dme_softc *, struct mbuf **);
static int pkt_write_1(struct dme_softc *, struct mbuf *);
#define PKT_READ(ii,m) (*(ii)->sc_pkt_read)((ii),(m))
#define PKT_WRITE(ii,m) (*(ii)->sc_pkt_write)((ii),(m))

#define ETHER_IS_ONE(x) \
	   (((x)[0] & (x)[1] & (x)[2] & (x)[3] & (x)[4] & (x)[5]) == 255)
#define ETHER_IS_ZERO(x) \
	   (((x)[0] | (x)[1] | (x)[2] | (x)[3] | (x)[4] | (x)[5]) == 0)

int
dme_attach(struct dme_softc *sc, const uint8_t *notusedanymore)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	struct ifmedia *ifm = &mii->mii_media;
	uint8_t b[2];
	uint16_t io_mode;
	uint8_t enaddr[ETHER_ADDR_LEN];
	prop_dictionary_t dict;
	prop_data_t ea;

	dme_read_c(sc, DM9000_VID0, b, 2);
	sc->sc_vendor_id = le16toh((uint16_t)b[1] << 8 | b[0]);
	dme_read_c(sc, DM9000_PID0, b, 2);
	sc->sc_product_id = le16toh((uint16_t)b[1] << 8 | b[0]);

	/* TODO: Check the vendor ID as well */
	if (sc->sc_product_id != 0x9000) {
		panic("dme_attach: product id mismatch (0x%hx != 0x9000)",
		    sc->sc_product_id);
	}
#if 1 || DM9000_DEBUG
	{
		dme_read_c(sc, DM9000_PAB0, enaddr, 6);
		aprint_normal_dev(sc->sc_dev,
		    "DM9000 was configured with MAC address: %s\n",
		    ether_sprintf(enaddr));
	}
#endif
	dict = device_properties(sc->sc_dev);
	ea = (dict) ? prop_dictionary_get(dict, "mac-address") : NULL;
	if (ea != NULL) {
	       /*
		 * If the MAC address is overridden by a device property,
		 * use that.
		 */
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(enaddr, prop_data_value(ea), ETHER_ADDR_LEN);
		aprint_debug_dev(sc->sc_dev, "got MAC address!\n");
	} else {
		/*
		 * If we did not get an externaly configure address,
		 * try to read one from the current setup, before
		 * resetting the chip.
		 */
		dme_read_c(sc, DM9000_PAB0, enaddr, 6);
		if (ETHER_IS_ONE(enaddr) || ETHER_IS_ZERO(enaddr)) {
			/* make a random MAC address */
			uint32_t maclo = 0x00f2 | (cprng_strong32() << 16);
			uint32_t machi = cprng_strong32();
			enaddr[0] = maclo;
			enaddr[1] = maclo >> 8;
			enaddr[2] = maclo >> 16;
			enaddr[3] = maclo >> 26;
			enaddr[4] = machi;
			enaddr[5] = machi >> 8;
		}
	}
	/* TODO: perform explicit EEPROM read op if it's availble */

	dme_reset(sc);

	mii->mii_ifp = ifp;
	mii->mii_readreg = mii_readreg;
	mii->mii_writereg = mii_writereg;
	mii->mii_statchg = mii_statchg;

	/* assume davicom PHY at 1. ext PHY could be hooked but only at 0-3 */
	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(ifm, 0, ether_mediachange, ether_mediastatus);
	mii_attach(sc->sc_dev, mii, 0xffffffff, 1 /* PHY 1 */,
		MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(ifm, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(ifm, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(ifm, IFM_ETHER | IFM_AUTO);
	ifm->ifm_media = ifm->ifm_cur->ifm_media;

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST;
	ifp->if_init = dme_init;
	ifp->if_start = dme_start;
	ifp->if_stop = dme_stop;
	ifp->if_ioctl = dme_ioctl;
	ifp->if_watchdog = NULL; /* no watchdog used */
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
	if_deferred_start_init(ifp, NULL);

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
            RND_TYPE_NET, RND_FLAG_DEFAULT);

	/* might be unnecessary as link change interrupt works well */
	callout_init(&sc->sc_link_callout, 0);
	callout_setfunc(&sc->sc_link_callout, phy_tick, sc);

	io_mode = (dme_read(sc, DM9000_ISR) &
	    DM9000_IOMODE_MASK) >> DM9000_IOMODE_SHIFT;

	/* frame body read/write ops in 2 byte quantity or byte-wise. */
	DPRINTF(("DM9000 Operation Mode: "));
	switch (io_mode) {
	case DM9000_MODE_8BIT:
		DPRINTF(("8-bit mode"));
		sc->sc_data_width = 1;
		sc->sc_pkt_write = pkt_write_1;
		sc->sc_pkt_read = pkt_read_1;
		break;
	case DM9000_MODE_16BIT:
		DPRINTF(("16-bit mode"));
		sc->sc_data_width = 2;
		sc->sc_pkt_write = pkt_write_2;
		sc->sc_pkt_read = pkt_read_2;
		break;
	case DM9000_MODE_32BIT:
		DPRINTF(("32-bit mode"));
		sc->sc_data_width = 4;
		panic("32bit mode is unsupported\n");
		break;
	default:
		DPRINTF(("Invalid mode"));
		break;
	}
	DPRINTF(("\n"));

	return 0;
}

int
dme_detach(struct dme_softc *sc)
{
	return 0;
}

/* Software Initialize/Reset of the DM9000 */
static void
dme_reset(struct dme_softc *sc)
{
	uint8_t misc;

	/* We only re-initialized the PHY in this function the first time it is
	 * called. */
	if (!sc->sc_phy_initialized) {
		/* PHY Reset */
		mii_writereg(sc->sc_dev, 1, MII_BMCR, BMCR_RESET);

		/* PHY Power Down */
		misc = dme_read(sc, DM9000_GPR);
		dme_write(sc, DM9000_GPR, misc | DM9000_GPR_PHY_PWROFF);
	}

	/* Reset the DM9000 twice, as described in section 2 of the Programming
	 * Guide.
	 * The PHY is initialized and enabled between those two resets.
	 */

	/* Software Reset */
	dme_write(sc, DM9000_NCR,
	    DM9000_NCR_RST | DM9000_NCR_LBK_MAC_INTERNAL);
	delay(20);
	dme_write(sc, DM9000_NCR, 0x0);

	if (!sc->sc_phy_initialized) {
		/* PHY Enable */
		misc = dme_read(sc, DM9000_GPR);
		dme_write(sc, DM9000_GPR, misc & ~DM9000_GPR_PHY_PWROFF);
		misc = dme_read(sc, DM9000_GPCR);
		dme_write(sc, DM9000_GPCR, misc | DM9000_GPCR_GPIO0_OUT);

		dme_write(sc, DM9000_NCR,
		    DM9000_NCR_RST | DM9000_NCR_LBK_MAC_INTERNAL);
		delay(20);
		dme_write(sc, DM9000_NCR, 0x0);
	}

	/* Select internal PHY, no wakeup event, no collosion mode,
	 * normal loopback mode.
	 */
	dme_write(sc, DM9000_NCR, DM9000_NCR_LBK_NORMAL);

	/* Will clear TX1END, TX2END, and WAKEST fields by reading DM9000_NSR*/
	dme_read(sc, DM9000_NSR);

	/* Enable wraparound of read/write pointer, frame received latch,
	 * and frame transmitted latch.
	 */
	dme_write(sc, DM9000_IMR,
	    DM9000_IMR_PAR | DM9000_IMR_PRM | DM9000_IMR_PTM);

	dme_write(sc, DM9000_RCR,
	    DM9000_RCR_DIS_CRC | DM9000_RCR_DIS_LONG | DM9000_RCR_WTDIS);

	sc->sc_phy_initialized = 1;
}

static int
dme_init(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;

	dme_stop(ifp, 0);
	dme_reset(sc);
	dme_write_c(sc, DM9000_PAB0, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
	dme_set_rcvfilt(sc);
	(void)ether_mediachange(ifp);

	sc->txbusy = sc->txready = 0;

	ifp->if_flags |= IFF_RUNNING;
	callout_schedule(&sc->sc_link_callout, hz);

	return 0;
}

/* Configure multicast filter */
static void
dme_set_rcvfilt(struct dme_softc *sc)
{
	struct ethercom	*ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint8_t mchash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; /* 64bit mchash */
	uint32_t h = 0;
	int rcr;

	rcr = dme_read(sc, DM9000_RCR);
	rcr &= ~(DM9000_RCR_PRMSC | DM9000_RCR_ALL);
	dme_write(sc, DM9000_RCR, rcr &~ DM9000_RCR_RXEN);

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		rcr |= DM9000_RCR_PRMSC;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcpy(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			memset(mchash, 0xff, sizeof(mchash)); /* necessary? */
			/* accept all mulicast frame */
			rcr |= DM9000_RCR_ALL;
			goto update;
		}
		h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN) & 0x3f;
		/* 3(5:3) and 3(2:0) sampling to have uint8_t[8] */
		mchash[h / 8] |= 1 << (h % 8);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	/* DM9000 receive filter is always on */
	mchash[7] |= 0x80; /* to catch bcast frame */
 update:
	dme_write_c(sc, DM9000_MAB0, mchash, sizeof(mchash));
	dme_write(sc, DM9000_RCR, rcr | DM9000_RCR_RXEN);
	return;
}

void
lnkchg(struct dme_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ifmediareq ifmr;

	ether_mediastatus(ifp, &ifmr);
}

static void
mii_statchg(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;
	uint8_t fcr, ncr;

#if 0
	const uint8_t Mbps[2] = { 10, 100 };
	uint8_t nsr = dme_read(sc, DM9000_NSR);
	int spd = Mbps[!!(nsr & DM9000_NSR_SPEED)];
	/* speed/duplexity available also in reg 0x11 of internal PHY */
	if (nsr & DM9000_NSR_LINKST)
		printf("link up,spd%d", spd);
	else
		printf("link down");

	/* show resolved mii(4) parameters */
	printf("MII spd%d",
	    (int)(sc->sc_ethercom.ec_if.if_baudrate / IF_Mbps(1)));
	if (mii->mii_media_active & IFM_FDX)
		printf(",full-duplex");
	printf("\n");
#endif

	/* Adjust duplexity and PAUSE flow control. */
	fcr = dme_read(sc, DM9000_FCR) &~ DM9000_FCR_FLCE;
	ncr = dme_read(sc, DM9000_NCR) &~ DM9000_NCR_FDX;
	if ((mii->mii_media_active & IFM_FDX)
	    && (mii->mii_media_active & IFM_FLOW)) {
		fcr |= DM9000_FCR_FLCE;
		ncr |= DM9000_NCR_FDX;
	}
	dme_write(sc, DM9000_FCR, fcr);
	dme_write(sc, DM9000_NCR, ncr);
}

static void
phy_tick(void *arg)
{
	struct dme_softc *sc = arg;
	struct mii_data *mii = &sc->sc_mii;
	int s;

	s = splnet();
	mii_tick(mii);
	splx(s);

	callout_schedule(&sc->sc_link_callout, hz);
}

static int
mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct dme_softc *sc = device_private(self);

	if (phy != 1)
		return EINVAL;

	/* Select Register to read*/
	dme_write(sc, DM9000_EPAR, DM9000_EPAR_INT_PHY +
	    (reg & DM9000_EPAR_EROA_MASK));
	/* Select read operation (DM9000_EPCR_ERPRR) from the PHY */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_ERPRR + DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while (dme_read(sc, DM9000_EPCR) & DM9000_EPCR_ERRE)
		;

	/* Reset ERPRR-bit */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);

	*val = dme_read(sc, DM9000_EPDRL) | (dme_read(sc, DM9000_EPDRH) << 8);
	return 0;
}

static int
mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct dme_softc *sc = device_private(self);

	if (phy != 1)
		return EINVAL;

	/* Select Register to write */
	dme_write(sc, DM9000_EPAR, DM9000_EPAR_INT_PHY +
	    (reg & DM9000_EPAR_EROA_MASK));

	/* Write data to the two data registers */
	dme_write(sc, DM9000_EPDRL, val & 0xFF);
	dme_write(sc, DM9000_EPDRH, (val >> 8) & 0xFF);

	/* Select write operation (DM9000_EPCR_ERPRW) from the PHY */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_ERPRW + DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while (dme_read(sc, DM9000_EPCR) & DM9000_EPCR_ERRE)
		;

	/* Reset ERPRR-bit */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);

	return 0;
}

void
dme_stop(struct ifnet *ifp, int disable)
{
	struct dme_softc *sc = ifp->if_softc;

	/* Not quite sure what to do when called with disable == 0 */
	if (disable) {
		/* Disable RX */
		dme_write(sc, DM9000_RCR, 0x0);
	}
	mii_down(&sc->sc_mii);
	callout_stop(&sc->sc_link_callout);

	ifp->if_flags &= ~IFF_RUNNING;
	ifp->if_timer = 0;
}

static void
dme_start(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		return;
	}
	if (!sc->txready) {
		dme_prepare(ifp);
	}
	if (sc->txbusy) {
		/*
		 * We need to wait until the current frame has
		 * been transmitted.
		 */
		return;
	}
	if (sc->txready) {
		/* We are ready to transmit right away */
		dme_transmit(ifp);
	}
	dme_prepare(ifp); /* Prepare next one */
}

/* Prepare data to be transmitted (i.e. dequeue and load it into the DM9000) */
static void
dme_prepare(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;
	uint16_t length;
	struct mbuf *m;

	KASSERT(!sc->txready);

	IFQ_DEQUEUE(&ifp->if_snd, m);
	if (m == NULL) {
		TX_DPRINTF(("dme_prepare: Nothing to transmit\n"));
		return; /* Nothing to transmit */
	}

	/* Element has now been removed from the queue, so we better send it */

	bpf_mtap(ifp, m, BPF_D_OUT);

	/* Setup the DM9000 to accept the writes, and then write each buf in
	   the chain. */

	TX_DATA_DPRINTF(("dme_prepare: Writing data: "));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->dme_io, DM9000_MWCMD);
	length = PKT_WRITE(sc, m);
	bpf_mtap(ifp, m, BPF_D_OUT);
	TX_DATA_DPRINTF(("\n"));

	if (length % sc->sc_data_width != 0)
		panic("dme_prepare: length is not compatible with IO_MODE");

	sc->txready_length = length;
	sc->txready = 1;
	m_freem(m);
}

/* Transmit prepared data */
static void
dme_transmit(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;

	TX_DPRINTF(("dme_transmit: PRE: txready: %d, txbusy: %d\n",
		sc->txready, sc->txbusy));

	/* prime frame length first */
	dme_write(sc, DM9000_TXPLL, sc->txready_length & 0xff);
	dme_write(sc, DM9000_TXPLH, (sc->txready_length >> 8) & 0xff);
	/* read isr next */
	dme_read(sc, DM9000_ISR);
	/* finally issue a request to send */
	dme_write(sc, DM9000_TCR, DM9000_TCR_TXREQ);
	sc->txready = 0;
	sc->txbusy = 1;
	sc->txready_length = 0;
}

/* Receive data */
static void
dme_receive(struct ifnet *ifp)
{
	struct dme_softc *sc = ifp->if_softc;
	struct mbuf *m;
	uint8_t avail, rsr;

	DPRINTF(("inside dme_receive\n"));

	/* frame has just arrived, retrieve it */
	/* called right after Rx frame available interrupt */
	do {
		/* "no increment" read to get the avail byte without
		   moving past it. */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->dme_io,
			DM9000_MRCMDX);
		/* Read twice */
		avail = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
		avail = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
		avail &= 03;	/* 1:0 we only want these bits */
		if (avail == 01) {
			/* Read with address increment. */
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->dme_io,
				DM9000_MRCMD);
			rsr = PKT_READ(sc, &m);
			if (m == NULL) {
				/* failed to allocate a receive buffer */
				RX_DPRINTF(("dme_receive: "
					"Error allocating buffer\n"));
				if_statinc(ifp, if_ierrors);
				continue;
			}
			if (rsr & (DM9000_RSR_CE | DM9000_RSR_PLE)) {
				/* Error while receiving the frame,
				 * discard it and keep track of counters
				 */
				RX_DPRINTF(("dme_receive: "
					"Error reciving frame\n"));
				if_statinc(ifp, if_ierrors);
				continue;
			}
			if (rsr & DM9000_RSR_LCS) {
				if_statinc(ifp, if_collisions);
				continue;
			}
			/* pick and forward this frame to ifq */
			if_percpuq_enqueue(ifp->if_percpuq, m);
		} else if (avail != 00) {
			/* Should this be logged somehow? */
			printf("%s: Resetting chip\n",
			       device_xname(sc->sc_dev));
			dme_reset(sc);
			break;
		}
	} while (avail == 01);
	/* frame received successfully */
}

int
dme_intr(void *arg)
{
	struct dme_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t isr, nsr, tsr;

	DPRINTF(("dme_intr: Begin\n"));

	/* Disable interrupts */
	dme_write(sc, DM9000_IMR, DM9000_IMR_PAR);

	isr = dme_read(sc, DM9000_ISR);
	dme_write(sc, DM9000_ISR, isr); /* write to clear */

	if (isr & DM9000_ISR_PRS) {
		KASSERT(ifp->if_flags & IFF_RUNNING);
		dme_receive(ifp);
	}
	if (isr & DM9000_ISR_LNKCHNG)
		lnkchg(sc);
	if (isr & DM9000_ISR_PTS) {
		tsr = 0x01; /* Initialize to an error value */

		/* A frame has been transmitted */
		sc->txbusy = 0;

		nsr = dme_read(sc, DM9000_NSR);
		if (nsr & DM9000_NSR_TX1END) {
			tsr = dme_read(sc, DM9000_TSR1);
			TX_DPRINTF(("dme_intr: Sent using channel 0\n"));
		} else if (nsr & DM9000_NSR_TX2END) {
			tsr = dme_read(sc, DM9000_TSR2);
			TX_DPRINTF(("dme_intr: Sent using channel 1\n"));
		}

		if (tsr == 0x0) {
			/* Frame successfully sent */
			if_statinc(ifp, if_opackets);
		} else {
			if_statinc(ifp, if_oerrors);
		}

		/* If we have nothing ready to transmit, prepare something */
		if (!sc->txready)
			dme_prepare(ifp);

		if (sc->txready)
			dme_transmit(ifp);

		/* Prepare the next frame */
		dme_prepare(ifp);

		if_schedule_deferred_start(ifp);
	}

	/* Enable interrupts again */
	dme_write(sc, DM9000_IMR,
	    DM9000_IMR_PAR | DM9000_IMR_PRM | DM9000_IMR_PTM);

	DPRINTF(("dme_intr: End\n"));

	return (isr != 0);
}

static int
dme_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct dme_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifmedia *ifm = &sc->sc_mii.mii_media;
	int s, error;

	s = splnet();
	switch (cmd) {
	case SIOCSIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0)
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				ifr->ifr_media |=
					IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
		}
		error = ifmedia_ioctl(ifp, ifr, ifm, cmd);
		break;
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;
		error = 0;
		if (cmd == SIOCSIFCAP)
			error = if_init(ifp);
		else if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if (ifp->if_flags && IFF_RUNNING) {
			/* Address list has changed, reconfigure filter */
			dme_set_rcvfilt(sc);
		}
		break;
	}
	splx(s);
	return error;
}

static struct mbuf *
dme_alloc_receive_buffer(struct ifnet *ifp, unsigned int frame_length)
{
	struct dme_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int pad, quantum;

	quantum = sc->sc_data_width;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	m_set_rcvif(m, ifp);
	/* Ensure that we always allocate an even number of
	 * bytes in order to avoid writing beyond the buffer
	 */
	m->m_pkthdr.len = frame_length + (frame_length % quantum);
	pad = ALIGN(sizeof(struct ether_header)) -
		sizeof(struct ether_header);
	/* All our frames have the CRC attached */
	m->m_flags |= M_HASFCS;
	if (m->m_pkthdr.len + pad > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
	}

	m->m_data += pad;
	m->m_len = frame_length + (frame_length % quantum);

	return m;
}

static int
pkt_write_2(struct dme_softc *sc, struct mbuf *bufChain)
{
	int left_over_count = 0; /* Number of bytes from previous mbuf, which
				    need to be written with the next.*/
	uint16_t left_over_buf = 0;
	int length = 0;
	struct mbuf *buf;
	uint8_t *write_ptr;

	/* We expect that the DM9000 has been setup to accept writes before
	   this function is called. */

	for (buf = bufChain; buf != NULL; buf = buf->m_next) {
		int to_write = buf->m_len;

		length += to_write;

		write_ptr = buf->m_data;
		while (to_write > 0 ||
		    (buf->m_next == NULL && left_over_count > 0)) {
			if (left_over_count > 0) {
				uint8_t b = 0;
				DPRINTF(("pkt_write_16: "
					 "Writing left over byte\n"));

				if (to_write > 0) {
					b = *write_ptr;
					to_write--;
					write_ptr++;

					DPRINTF(("Took single byte\n"));
				} else {
					DPRINTF(("Leftover in last run\n"));
					length++;
				}

				/* Does shift direction depend on endianness? */
				left_over_buf = left_over_buf | (b << 8);

				bus_space_write_2(sc->sc_iot, sc->sc_ioh,
						  sc->dme_data, left_over_buf);
				TX_DATA_DPRINTF(("%02X ", left_over_buf));
				left_over_count = 0;
			} else if ((long)write_ptr % 2 != 0) {
				/* Misaligned data */
				DPRINTF(("pkt_write_16: "
					 "Detected misaligned data\n"));
				left_over_buf = *write_ptr;
				left_over_count = 1;
				write_ptr++;
				to_write--;
			} else {
				int i;
				uint16_t *dptr = (uint16_t *)write_ptr;

				/* A block of aligned data. */
				for (i = 0; i < to_write / 2; i++) {
					/* buf will be half-word aligned
					 * all the time
					 */
					bus_space_write_2(sc->sc_iot,
					    sc->sc_ioh, sc->dme_data, *dptr);
					TX_DATA_DPRINTF(("%02X %02X ",
					    *dptr & 0xFF, (*dptr >> 8) & 0xFF));
					dptr++;
				}

				write_ptr += i * 2;
				if (to_write % 2 != 0) {
					DPRINTF(("pkt_write_16: "
						 "to_write %% 2: %d\n",
						 to_write % 2));
					left_over_count = 1;
					/* XXX: Does this depend on
					 * the endianness?
					 */
					left_over_buf = *write_ptr;

					write_ptr++;
					to_write--;
					DPRINTF(("pkt_write_16: "
						 "to_write (after): %d\n",
						 to_write));
					DPRINTF(("pkt_write_16: i * 2: %d\n",
						 i*2));
				}
				to_write -= i * 2;
			}
		} /* while (...) */
	} /* for (...) */

	return length;
}

static int
pkt_read_2(struct dme_softc *sc, struct mbuf **outBuf)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t rx_status;
	struct mbuf *m;
	uint16_t data;
	uint16_t frame_length;
	uint16_t i;
	uint16_t *buf;

	data = bus_space_read_2(sc->sc_iot, sc->sc_ioh, sc->dme_data);
	rx_status = data & 0xFF;

	frame_length = bus_space_read_2(sc->sc_iot,
					sc->sc_ioh, sc->dme_data);
	if (frame_length > ETHER_MAX_LEN) {
		printf("Got frame of length: %d\n", frame_length);
		printf("ETHER_MAX_LEN is: %d\n", ETHER_MAX_LEN);
		panic("Something is rotten");
	}
	RX_DPRINTF(("dme_receive: rx_statux: 0x%x, frame_length: %d\n",
		rx_status, frame_length));

	m = dme_alloc_receive_buffer(ifp, frame_length);
	if (m == NULL) {
		/*
		 * didn't get a receive buffer, so we read the rest of the
		 * frame, throw it away and return an error
		 */
		for (i = 0; i < frame_length; i += 2) {
			data = bus_space_read_2(sc->sc_iot,
					sc->sc_ioh, sc->dme_data);
		}
		*outBuf = NULL;
		return 0;
	}

	buf = mtod(m, uint16_t*);

	RX_DPRINTF(("dme_receive: "));

	for (i = 0; i < frame_length; i += 2) {
		data = bus_space_read_2(sc->sc_iot,
					sc->sc_ioh, sc->dme_data);
		if ( (frame_length % 2 != 0) &&
		     (i == frame_length - 1) ) {
			data = data & 0xff;
			RX_DPRINTF((" L "));
		}
		*buf = data;
		buf++;
		RX_DATA_DPRINTF(("%02X %02X ", data & 0xff,
				 (data >> 8) & 0xff));
	}

	RX_DATA_DPRINTF(("\n"));
	RX_DPRINTF(("Read %d bytes\n", i));

	*outBuf = m;
	return rx_status;
}

static int
pkt_write_1(struct dme_softc *sc, struct mbuf *bufChain)
{
	int length = 0, i;
	struct mbuf *buf;
	uint8_t *write_ptr;

	/*
	 * We expect that the DM9000 has been setup to accept writes before
	 * this function is called.
	 */

	for (buf = bufChain; buf != NULL; buf = buf->m_next) {
		int to_write = buf->m_len;

		length += to_write;

		write_ptr = buf->m_data;
		for (i = 0; i < to_write; i++) {
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    sc->dme_data, *write_ptr);
			write_ptr++;
		}
	} /* for (...) */

	return length;
}

static int
pkt_read_1(struct dme_softc *sc, struct mbuf **outBuf)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t rx_status;
	struct mbuf *m;
	uint8_t *buf;
	uint16_t frame_length;
	uint16_t i, reg;
	uint8_t data;

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
	reg |= bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data) << 8;
	rx_status = reg & 0xFF;

	reg = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
	reg |= bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data) << 8;
	frame_length = reg;

	if (frame_length > ETHER_MAX_LEN) {
		printf("Got frame of length: %d\n", frame_length);
		printf("ETHER_MAX_LEN is: %d\n", ETHER_MAX_LEN);
		panic("Something is rotten");
	}
	RX_DPRINTF(("dme_receive: "
		    "rx_statux: 0x%x, frame_length: %d\n",
		    rx_status, frame_length));

	m = dme_alloc_receive_buffer(ifp, frame_length);
	if (m == NULL) {
		/*
		 * didn't get a receive buffer, so we read the rest of the
		 * frame, throw it away and return an error
		 */
		for (i = 0; i < frame_length; i++ ) {
			data = bus_space_read_2(sc->sc_iot,
					sc->sc_ioh, sc->dme_data);
		}
		*outBuf = NULL;
		return 0;
	}

	buf = mtod(m, uint8_t *);

	RX_DPRINTF(("dme_receive: "));
	for (i = 0; i< frame_length; i += 1) {
		data = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
		*buf = data;
		buf++;
		RX_DATA_DPRINTF(("%02X ", data));
	}

	RX_DATA_DPRINTF(("\n"));
	RX_DPRINTF(("Read %d bytes\n", i));

	*outBuf = m;
	return rx_status;
}
