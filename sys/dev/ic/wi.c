/*	$NetBSD: wi.c,v 1.155 2004/03/17 17:00:34 dyoung Exp $	*/

/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 * Lucent WaveLAN/IEEE 802.11 PCMCIA driver for NetBSD.
 *
 * Original FreeBSD driver written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The WaveLAN/IEEE adapter is the second generation of the WaveLAN
 * from Lucent. Unlike the older cards, the new ones are programmed
 * entirely via a firmware-driven controller called the Hermes.
 * Unfortunately, Lucent will not release the Hermes programming manual
 * without an NDA (if at all). What they do release is an API library
 * called the HCF (Hardware Control Functions) which is supposed to
 * do the device-specific operations of a device driver for you. The
 * publically available version of the HCF library (the 'HCF Light') is 
 * a) extremely gross, b) lacks certain features, particularly support
 * for 802.11 frames, and c) is contaminated by the GNU Public License.
 *
 * This driver does not use the HCF or HCF Light at all. Instead, it
 * programs the Hermes controller directly, using information gleaned
 * from the HCF Light code and corresponding documentation.
 *
 * This driver supports both the PCMCIA and ISA versions of the
 * WaveLAN/IEEE cards. Note however that the ISA card isn't really
 * anything of the sort: it's actually a PCMCIA bridge adapter
 * that fits into an ISA slot, into which a PCMCIA WaveLAN card is
 * inserted. Consequently, you need to use the pccard support for
 * both the ISA and PCMCIA adapters.
 */

/*
 * FreeBSD driver ported to NetBSD by Bill Sommerfeld in the back of the
 * Oslo IETF plenary meeting.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wi.c,v 1.155 2004/03/17 17:00:34 dyoung Exp $");

#define WI_HERMES_AUTOINC_WAR	/* Work around data write autoinc bug. */
#define WI_HERMES_STATS_WAR	/* Work around stats counter bug. */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/proc.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_compat.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_rssadapt.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

static int  wi_init(struct ifnet *);
static void wi_stop(struct ifnet *, int);
static void wi_start(struct ifnet *);
static int  wi_reset(struct wi_softc *);
static void wi_watchdog(struct ifnet *);
static int  wi_ioctl(struct ifnet *, u_long, caddr_t);
static int  wi_media_change(struct ifnet *);
static void wi_media_status(struct ifnet *, struct ifmediareq *);

static struct ieee80211_node *wi_node_alloc(struct ieee80211com *);
static void wi_node_copy(struct ieee80211com *, struct ieee80211_node *,
    const struct ieee80211_node *);
static void wi_node_free(struct ieee80211com *, struct ieee80211_node *);

static void wi_raise_rate(struct ieee80211com *, struct ieee80211_rssdesc *);
static void wi_lower_rate(struct ieee80211com *, struct ieee80211_rssdesc *);
static void wi_choose_rate(struct ieee80211com *, struct ieee80211_node *,
    struct ieee80211_frame *, u_int);
static void wi_rssadapt_updatestats_cb(void *, struct ieee80211_node *);
static void wi_rssadapt_updatestats(void *);

static void wi_rx_intr(struct wi_softc *);
static void wi_txalloc_intr(struct wi_softc *);
static void wi_tx_intr(struct wi_softc *);
static void wi_tx_ex_intr(struct wi_softc *);
static void wi_info_intr(struct wi_softc *);

static int  wi_get_cfg(struct ifnet *, u_long, caddr_t);
static int  wi_set_cfg(struct ifnet *, u_long, caddr_t);
static int  wi_cfg_txrate(struct wi_softc *);
static int  wi_write_txrate(struct wi_softc *, int);
static int  wi_write_wep(struct wi_softc *);
static int  wi_write_multi(struct wi_softc *);
static int  wi_alloc_fid(struct wi_softc *, int, int *);
static void wi_read_nicid(struct wi_softc *);
static int  wi_write_ssid(struct wi_softc *, int, u_int8_t *, int);

static int  wi_cmd(struct wi_softc *, int, int, int, int);
static int  wi_seek_bap(struct wi_softc *, int, int);
static int  wi_read_bap(struct wi_softc *, int, int, void *, int);
static int  wi_write_bap(struct wi_softc *, int, int, void *, int);
static int  wi_mwrite_bap(struct wi_softc *, int, int, struct mbuf *, int);
static int  wi_read_rid(struct wi_softc *, int, void *, int *);
static int  wi_write_rid(struct wi_softc *, int, void *, int);

static int  wi_newstate(struct ieee80211com *, enum ieee80211_state, int);
static int  wi_set_tim(struct ieee80211com *, int, int);

static int  wi_scan_ap(struct wi_softc *, u_int16_t, u_int16_t);
static void wi_scan_result(struct wi_softc *, int, int);

static void wi_dump_pkt(struct wi_frame *, struct ieee80211_node *, int rssi);

static inline int
wi_write_val(struct wi_softc *sc, int rid, u_int16_t val)
{

	val = htole16(val);
	return wi_write_rid(sc, rid, &val, sizeof(val));
}

static	struct timeval lasttxerror;	/* time of last tx error msg */
static	int curtxeps = 0;		/* current tx error msgs/sec */
static	int wi_txerate = 0;		/* tx error rate: max msgs/sec */

#ifdef WI_DEBUG
int wi_debug = 0;

#define	DPRINTF(X)	if (wi_debug) printf X
#define	DPRINTF2(X)	if (wi_debug > 1) printf X
#define	IFF_DUMPPKTS(_ifp) \
	(((_ifp)->if_flags & (IFF_DEBUG|IFF_LINK2)) == (IFF_DEBUG|IFF_LINK2))
#else
#define	DPRINTF(X)
#define	DPRINTF2(X)
#define	IFF_DUMPPKTS(_ifp)	0
#endif

#define WI_INTRS	(WI_EV_RX | WI_EV_ALLOC | WI_EV_INFO)

struct wi_card_ident
wi_card_ident[] = {
	/* CARD_ID			CARD_NAME		FIRM_TYPE */
	{ WI_NIC_LUCENT_ID,		WI_NIC_LUCENT_STR,	WI_LUCENT },
	{ WI_NIC_SONY_ID,		WI_NIC_SONY_STR,	WI_LUCENT },
	{ WI_NIC_LUCENT_EMB_ID,		WI_NIC_LUCENT_EMB_STR,	WI_LUCENT },
	{ WI_NIC_EVB2_ID,		WI_NIC_EVB2_STR,	WI_INTERSIL },
	{ WI_NIC_HWB3763_ID,		WI_NIC_HWB3763_STR,	WI_INTERSIL },
	{ WI_NIC_HWB3163_ID,		WI_NIC_HWB3163_STR,	WI_INTERSIL },
	{ WI_NIC_HWB3163B_ID,		WI_NIC_HWB3163B_STR,	WI_INTERSIL },
	{ WI_NIC_EVB3_ID,		WI_NIC_EVB3_STR,	WI_INTERSIL },
	{ WI_NIC_HWB1153_ID,		WI_NIC_HWB1153_STR,	WI_INTERSIL },
	{ WI_NIC_P2_SST_ID,		WI_NIC_P2_SST_STR,	WI_INTERSIL },
	{ WI_NIC_EVB2_SST_ID,		WI_NIC_EVB2_SST_STR,	WI_INTERSIL },
	{ WI_NIC_3842_EVA_ID,		WI_NIC_3842_EVA_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCMCIA_AMD_ID,	WI_NIC_3842_PCMCIA_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCMCIA_SST_ID,	WI_NIC_3842_PCMCIA_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCMCIA_ATM_ID,	WI_NIC_3842_PCMCIA_STR,	WI_INTERSIL },
	{ WI_NIC_3842_MINI_AMD_ID,	WI_NIC_3842_MINI_STR,	WI_INTERSIL },
	{ WI_NIC_3842_MINI_SST_ID,	WI_NIC_3842_MINI_STR,	WI_INTERSIL },
	{ WI_NIC_3842_MINI_ATM_ID,	WI_NIC_3842_MINI_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCI_AMD_ID,	WI_NIC_3842_PCI_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCI_SST_ID,	WI_NIC_3842_PCI_STR,	WI_INTERSIL },
	{ WI_NIC_3842_PCI_ATM_ID,	WI_NIC_3842_PCI_STR,	WI_INTERSIL },
	{ WI_NIC_P3_PCMCIA_AMD_ID,	WI_NIC_P3_PCMCIA_STR,	WI_INTERSIL },
	{ WI_NIC_P3_PCMCIA_SST_ID,	WI_NIC_P3_PCMCIA_STR,	WI_INTERSIL },
	{ WI_NIC_P3_MINI_AMD_ID,	WI_NIC_P3_MINI_STR,	WI_INTERSIL },
	{ WI_NIC_P3_MINI_SST_ID,	WI_NIC_P3_MINI_STR,	WI_INTERSIL },
	{ 0,	NULL,	0 },
};

int
wi_attach(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int chan, nrate, buflen;
	u_int16_t val, chanavail;
 	struct {
 		u_int16_t nrates;
 		char rates[IEEE80211_RATE_SIZE];
 	} ratebuf;
	static const u_int8_t empty_macaddr[IEEE80211_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int s;

	s = splnet();

	/* Make sure interrupts are disabled. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, ~0);

	sc->sc_invalid = 0;

	/* Reset the NIC. */
	if (wi_reset(sc) != 0) {
		sc->sc_invalid = 1;
		splx(s);
		return 1;
	}

	buflen = IEEE80211_ADDR_LEN;
	if (wi_read_rid(sc, WI_RID_MAC_NODE, ic->ic_myaddr, &buflen) != 0 ||
	    IEEE80211_ADDR_EQ(ic->ic_myaddr, empty_macaddr)) {
		printf(" could not get mac address, attach failed\n");
		splx(s);
		return 1;
	}

	printf(" 802.11 address %s\n", ether_sprintf(ic->ic_myaddr));

	/* Read NIC identification */
	wi_read_nicid(sc);

	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = wi_start;
	ifp->if_ioctl = wi_ioctl;
	ifp->if_watchdog = wi_watchdog;
	ifp->if_init = wi_init;
	ifp->if_stop = wi_stop;
	ifp->if_flags =
	    IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST | IFF_NOTRAILERS;
	IFQ_SET_READY(&ifp->if_snd);

	ic->ic_phytype = IEEE80211_T_DS;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_caps = IEEE80211_C_PMGT | IEEE80211_C_AHDEMO;
	ic->ic_state = IEEE80211_S_INIT;
	ic->ic_max_aid = WI_MAX_AID;

	/* Find available channel */
	buflen = sizeof(chanavail);
	if (wi_read_rid(sc, WI_RID_CHANNEL_LIST, &chanavail, &buflen) != 0)
		chanavail = htole16(0x1fff);	/* assume 1-11 */
	for (chan = 16; chan > 0; chan--) {
		if (!isset((u_int8_t*)&chanavail, chan - 1))
			continue;
		ic->ic_ibss_chan = &ic->ic_channels[chan];
		ic->ic_channels[chan].ic_freq =
		    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[chan].ic_flags = IEEE80211_CHAN_B;
	}

	/* Find default IBSS channel */
	buflen = sizeof(val);
	if (wi_read_rid(sc, WI_RID_OWN_CHNL, &val, &buflen) == 0) {
		chan = le16toh(val);
		if (isset((u_int8_t*)&chanavail, chan - 1))
			ic->ic_ibss_chan = &ic->ic_channels[chan];
	}
	if (ic->ic_ibss_chan == NULL)
		panic("%s: no available channel\n", sc->sc_dev.dv_xname);

	if (sc->sc_firmware_type == WI_LUCENT) {
		sc->sc_dbm_offset = WI_LUCENT_DBM_OFFSET;
	} else {
		buflen = sizeof(val);
		if ((sc->sc_flags & WI_FLAGS_HAS_DBMADJUST) &&
		    wi_read_rid(sc, WI_RID_DBM_ADJUST, &val, &buflen) == 0)
			sc->sc_dbm_offset = le16toh(val);
		else
			sc->sc_dbm_offset = WI_PRISM_DBM_OFFSET;
	}

	/*
	 * Set flags based on firmware version.
	 */
	switch (sc->sc_firmware_type) {
	case WI_LUCENT:
		sc->sc_flags |= WI_FLAGS_HAS_SYSSCALE;
#ifdef WI_HERMES_AUTOINC_WAR
		/* XXX: not confirmed, but never seen for recent firmware */
		if (sc->sc_sta_firmware_ver <  40000) {
			sc->sc_flags |= WI_FLAGS_BUG_AUTOINC;
		}
#endif
		/* RSS rate-adaptation is known to cause STA f/w
		 * 8.42.1 to lock up. STA f/w 8.70.1 and 7.28.1
		 * appear to work.  I suspect that most versions
		 * will work.
		 */
		switch (sc->sc_sta_firmware_ver) {
		case 0x084201:
			sc->sc_flags &= ~WI_FLAGS_RSSADAPTSTA;
			break;
		case 0x087001:
		case 0x072801:
		default:
			sc->sc_flags |= WI_FLAGS_RSSADAPTSTA;
			break;
		}
		if (sc->sc_sta_firmware_ver >= 60000)
			sc->sc_flags |= WI_FLAGS_HAS_MOR;
		if (sc->sc_sta_firmware_ver >= 60006) {
			ic->ic_caps |= IEEE80211_C_IBSS;
			ic->ic_caps |= IEEE80211_C_MONITOR;
		}
		sc->sc_ibss_port = 1;
		break;

	case WI_INTERSIL:
		sc->sc_flags |= WI_FLAGS_RSSADAPTSTA;
		sc->sc_flags |= WI_FLAGS_HAS_FRAGTHR;
		sc->sc_flags |= WI_FLAGS_HAS_ROAMING;
		sc->sc_flags |= WI_FLAGS_HAS_SYSSCALE;
		if (sc->sc_sta_firmware_ver > 10101)
			sc->sc_flags |= WI_FLAGS_HAS_DBMADJUST;
		if (sc->sc_sta_firmware_ver >= 800) {
			if (sc->sc_sta_firmware_ver != 10402)
				ic->ic_caps |= IEEE80211_C_HOSTAP;
			ic->ic_caps |= IEEE80211_C_IBSS;
			ic->ic_caps |= IEEE80211_C_MONITOR;
		}
		sc->sc_ibss_port = 0;
		sc->sc_alt_retry = 2;
		break;

	case WI_SYMBOL:
		sc->sc_flags |= WI_FLAGS_RSSADAPTSTA;
		sc->sc_flags |= WI_FLAGS_HAS_DIVERSITY;
		if (sc->sc_sta_firmware_ver >= 20000)
			ic->ic_caps |= IEEE80211_C_IBSS;
		sc->sc_ibss_port = 4;
		break;
	}

	/*
	 * Find out if we support WEP on this card.
	 */
	buflen = sizeof(val);
	if (wi_read_rid(sc, WI_RID_WEP_AVAIL, &val, &buflen) == 0 &&
	    val != htole16(0))
		ic->ic_caps |= IEEE80211_C_WEP;

	/* Find supported rates. */
	buflen = sizeof(ratebuf);
	if (wi_read_rid(sc, WI_RID_DATA_RATES, &ratebuf, &buflen) == 0) {
		nrate = le16toh(ratebuf.nrates);
		if (nrate > IEEE80211_RATE_SIZE)
			nrate = IEEE80211_RATE_SIZE;
		memcpy(ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates,
		    &ratebuf.rates[0], nrate);
		ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates = nrate;
	}
	buflen = sizeof(val);

	sc->sc_max_datalen = 2304;
	sc->sc_rts_thresh = 2347;
	sc->sc_frag_thresh = 2346;
	sc->sc_system_scale = 1;
	sc->sc_cnfauthmode = IEEE80211_AUTH_OPEN;
	sc->sc_roaming_mode = 1;

	callout_init(&sc->sc_rssadapt_ch);

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ieee80211_ifattach(ifp);

	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = wi_newstate;
	ic->ic_node_alloc = wi_node_alloc;
	ic->ic_node_free = wi_node_free;
	ic->ic_node_copy = wi_node_copy;
	ic->ic_set_tim = wi_set_tim;

	ieee80211_media_init(ifp, wi_media_change, wi_media_status);

#if NBPFILTER > 0
	bpfattach2(ifp, DLT_IEEE802_11_RADIO,
	    sizeof(struct ieee80211_frame) + 64, &sc->sc_drvbpf);
#endif

	memset(&sc->sc_rxtapu, 0, sizeof(sc->sc_rxtapu));
	sc->sc_rxtap.wr_ihdr.it_len = sizeof(sc->sc_rxtapu);
	sc->sc_rxtap.wr_ihdr.it_present = WI_RX_RADIOTAP_PRESENT;

	memset(&sc->sc_txtapu, 0, sizeof(sc->sc_txtapu));
	sc->sc_txtap.wt_ihdr.it_len = sizeof(sc->sc_txtapu);
	sc->sc_txtap.wt_ihdr.it_present = WI_TX_RADIOTAP_PRESENT;

	/* Attach is successful. */
	sc->sc_attached = 1;

	splx(s);
	return 0;
}

int
wi_detach(struct wi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	if (!sc->sc_attached)
		return 0;

	s = splnet();

	sc->sc_invalid = 1;
	wi_stop(ifp, 1);

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->sc_ic.ic_media, IFM_INST_ANY);

	ieee80211_ifdetach(ifp);
	if_detach(ifp);
	splx(s);
	return 0;
}

#ifdef __NetBSD__
int
wi_activate(struct device *self, enum devact act)
{
	struct wi_softc *sc = (struct wi_softc *)self;
	int rv = 0, s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ic.ic_if);
		break;
	}
	splx(s);
	return rv;
}

void
wi_power(struct wi_softc *sc, int why)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	s = splnet();
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		wi_stop(ifp, 1);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			wi_init(ifp);
			(void)wi_intr(sc);
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}
#endif /* __NetBSD__ */

void
wi_shutdown(struct wi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	if (sc->sc_attached)
		wi_stop(ifp, 1);
}

int
wi_intr(void *arg)
{
	int i;
	struct wi_softc	*sc = arg;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	u_int16_t status;

	if (sc->sc_enabled == 0 ||
	    (sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 ||
	    (ifp->if_flags & IFF_RUNNING) == 0)
		return 0;

	if ((ifp->if_flags & IFF_UP) == 0) {
		CSR_WRITE_2(sc, WI_INT_EN, 0);
		CSR_WRITE_2(sc, WI_EVENT_ACK, ~0);
		return 1;
	}

	/* This is superfluous on Prism, but Lucent breaks if we
	 * do not disable interrupts.
	 */
	CSR_WRITE_2(sc, WI_INT_EN, 0);

	/* maximum 10 loops per interrupt */
	for (i = 0; i < 10; i++) {
		/*
		 * Only believe a status bit when we enter wi_intr, or when
		 * the bit was "off" the last time through the loop. This is
		 * my strategy to avoid racing the hardware/firmware if I
		 * can re-read the event status register more quickly than
		 * it is updated.
		 */
		status = CSR_READ_2(sc, WI_EVENT_STAT);
		if ((status & WI_INTRS) == 0)
			break;

		if (status & WI_EV_RX)
			wi_rx_intr(sc);

		if (status & WI_EV_ALLOC)
			wi_txalloc_intr(sc);

		if (status & WI_EV_TX)
			wi_tx_intr(sc);

		if (status & WI_EV_TX_EXC)
			wi_tx_ex_intr(sc);

		if (status & WI_EV_INFO)
			wi_info_intr(sc);

		if ((ifp->if_flags & IFF_OACTIVE) == 0 &&
		    (sc->sc_flags & WI_FLAGS_OUTRANGE) == 0 &&
		    !IFQ_IS_EMPTY(&ifp->if_snd))
			wi_start(ifp);
	}

	/* re-enable interrupts */
	CSR_WRITE_2(sc, WI_INT_EN, WI_INTRS);

	return 1;
}

#define arraylen(a) (sizeof(a) / sizeof((a)[0]))

static void
wi_rssdescs_init(struct wi_rssdesc (*rssd)[WI_NTXRSS], wi_rssdescq_t *rssdfree)
{
	int i;
	SLIST_INIT(rssdfree);
	for (i = 0; i < arraylen(*rssd); i++) {
		SLIST_INSERT_HEAD(rssdfree, &(*rssd)[i], rd_next);
	}
}

static void
wi_rssdescs_reset(struct ieee80211com *ic, struct wi_rssdesc (*rssd)[WI_NTXRSS],
    wi_rssdescq_t *rssdfree, u_int8_t (*txpending)[IEEE80211_RATE_MAXSIZE])
{
	struct ieee80211_node *ni;
	int i;
	for (i = 0; i < arraylen(*rssd); i++) {
		ni = (*rssd)[i].rd_desc.id_node;
		(*rssd)[i].rd_desc.id_node = NULL;
		if (ni != NULL && (ic->ic_if.if_flags & IFF_DEBUG) != 0)
			printf("%s: cleaning outstanding rssadapt "
			    "descriptor for %s\n",
			    ic->ic_if.if_xname, ether_sprintf(ni->ni_macaddr));
		if (ni != NULL && ni != ic->ic_bss)
			ieee80211_free_node(ic, ni);
	}
	memset(*txpending, 0, sizeof(*txpending));
	wi_rssdescs_init(rssd, rssdfree);
}

static int
wi_init(struct ifnet *ifp)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct wi_joinreq join;
	int i;
	int error = 0, wasenabled;

	DPRINTF(("wi_init: enabled %d\n", sc->sc_enabled));
	wasenabled = sc->sc_enabled;
	if (!sc->sc_enabled) {
		if ((error = (*sc->sc_enable)(sc)) != 0)
			goto out;
		sc->sc_enabled = 1;
	} else
		wi_stop(ifp, 0);

	/* Symbol firmware cannot be initialized more than once */
	if (sc->sc_firmware_type != WI_SYMBOL || !wasenabled)
		if ((error = wi_reset(sc)) != 0)
			goto out;

	/* common 802.11 configuration */
	ic->ic_flags &= ~IEEE80211_F_IBSSON;
	sc->sc_flags &= ~WI_FLAGS_OUTRANGE;
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		wi_write_val(sc, WI_RID_PORTTYPE, WI_PORTTYPE_BSS);
		break;
	case IEEE80211_M_IBSS:
		wi_write_val(sc, WI_RID_PORTTYPE, sc->sc_ibss_port);
		ic->ic_flags |= IEEE80211_F_IBSSON;
		sc->sc_syn_timer = 5;
		ifp->if_timer = 1;
		break;
	case IEEE80211_M_AHDEMO:
		wi_write_val(sc, WI_RID_PORTTYPE, WI_PORTTYPE_ADHOC);
		break;
	case IEEE80211_M_HOSTAP:
		wi_write_val(sc, WI_RID_PORTTYPE, WI_PORTTYPE_HOSTAP);
		break;
	case IEEE80211_M_MONITOR:
		if (sc->sc_firmware_type == WI_LUCENT)
			wi_write_val(sc, WI_RID_PORTTYPE, WI_PORTTYPE_ADHOC);
		wi_cmd(sc, WI_CMD_TEST | (WI_TEST_MONITOR << 8), 0, 0, 0);
		break;
	}

	/* Intersil interprets this RID as joining ESS even in IBSS mode */
	if (sc->sc_firmware_type == WI_LUCENT &&
	    (ic->ic_flags & IEEE80211_F_IBSSON) && ic->ic_des_esslen > 0)
		wi_write_val(sc, WI_RID_CREATE_IBSS, 1);
	else
		wi_write_val(sc, WI_RID_CREATE_IBSS, 0);
	wi_write_val(sc, WI_RID_MAX_SLEEP, ic->ic_lintval);
	wi_write_ssid(sc, WI_RID_DESIRED_SSID, ic->ic_des_essid,
	    ic->ic_des_esslen);
	wi_write_val(sc, WI_RID_OWN_CHNL,
	    ieee80211_chan2ieee(ic, ic->ic_ibss_chan));
	wi_write_ssid(sc, WI_RID_OWN_SSID, ic->ic_des_essid, ic->ic_des_esslen);
	IEEE80211_ADDR_COPY(ic->ic_myaddr, LLADDR(ifp->if_sadl));
	wi_write_rid(sc, WI_RID_MAC_NODE, ic->ic_myaddr, IEEE80211_ADDR_LEN);
	wi_write_val(sc, WI_RID_PM_ENABLED,
	    (ic->ic_flags & IEEE80211_F_PMGTON) ? 1 : 0);

	/* not yet common 802.11 configuration */
	wi_write_val(sc, WI_RID_MAX_DATALEN, sc->sc_max_datalen);
	wi_write_val(sc, WI_RID_RTS_THRESH, sc->sc_rts_thresh);
	if (sc->sc_flags & WI_FLAGS_HAS_FRAGTHR)
		wi_write_val(sc, WI_RID_FRAG_THRESH, sc->sc_frag_thresh);

	/* driver specific 802.11 configuration */
	if (sc->sc_flags & WI_FLAGS_HAS_SYSSCALE)
		wi_write_val(sc, WI_RID_SYSTEM_SCALE, sc->sc_system_scale);
	if (sc->sc_flags & WI_FLAGS_HAS_ROAMING)
		wi_write_val(sc, WI_RID_ROAMING_MODE, sc->sc_roaming_mode);
	if (sc->sc_flags & WI_FLAGS_HAS_MOR)
		wi_write_val(sc, WI_RID_MICROWAVE_OVEN, sc->sc_microwave_oven);
	wi_cfg_txrate(sc);
	wi_write_ssid(sc, WI_RID_NODENAME, sc->sc_nodename, sc->sc_nodelen);

	if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
	    sc->sc_firmware_type == WI_INTERSIL) {
		wi_write_val(sc, WI_RID_OWN_BEACON_INT, ic->ic_lintval);
		wi_write_val(sc, WI_RID_BASIC_RATE, 0x03);   /* 1, 2 */
		wi_write_val(sc, WI_RID_SUPPORT_RATE, 0x0f); /* 1, 2, 5.5, 11 */
		wi_write_val(sc, WI_RID_DTIM_PERIOD, 1);
	}

	if (sc->sc_firmware_type == WI_INTERSIL)
		wi_write_val(sc, WI_RID_ALT_RETRY_COUNT, sc->sc_alt_retry);

	/*
	 * Initialize promisc mode.
	 *	Being in Host-AP mode causes a great
	 *	deal of pain if promiscuous mode is set.
	 *	Therefore we avoid confusing the firmware
	 *	and always reset promisc mode in Host-AP
	 *	mode.  Host-AP sees all the packets anyway.
	 */
	if (ic->ic_opmode != IEEE80211_M_HOSTAP &&
	    (ifp->if_flags & IFF_PROMISC) != 0) {
		wi_write_val(sc, WI_RID_PROMISC, 1);
	} else {
		wi_write_val(sc, WI_RID_PROMISC, 0);
	}

	/* Configure WEP. */
	if (ic->ic_caps & IEEE80211_C_WEP)
		wi_write_wep(sc);

	/* Set multicast filter. */
	wi_write_multi(sc);

	if (sc->sc_firmware_type != WI_SYMBOL || !wasenabled) {
		sc->sc_buflen = IEEE80211_MAX_LEN + sizeof(struct wi_frame);
		if (sc->sc_firmware_type == WI_SYMBOL)
			sc->sc_buflen = 1585;	/* XXX */
		for (i = 0; i < WI_NTXBUF; i++) {
			error = wi_alloc_fid(sc, sc->sc_buflen,
			    &sc->sc_txd[i].d_fid);
			if (error) {
				printf("%s: tx buffer allocation failed\n",
				    sc->sc_dev.dv_xname);
				goto out;
			}
			DPRINTF2(("wi_init: txbuf %d allocated %x\n", i,
			    sc->sc_txd[i].d_fid));
			sc->sc_txd[i].d_len = 0;
		}
	}
	sc->sc_txcur = sc->sc_txnext = 0;

	wi_rssdescs_init(&sc->sc_rssd, &sc->sc_rssdfree);

	/* Enable desired port */
	wi_cmd(sc, WI_CMD_ENABLE | sc->sc_portnum, 0, 0, 0);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ic->ic_state = IEEE80211_S_INIT;

	if (ic->ic_opmode == IEEE80211_M_AHDEMO ||
	    ic->ic_opmode == IEEE80211_M_MONITOR ||
	    ic->ic_opmode == IEEE80211_M_HOSTAP)
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);

	/* Enable interrupts */
	CSR_WRITE_2(sc, WI_INT_EN, WI_INTRS);

	if (!wasenabled &&
	    ic->ic_opmode == IEEE80211_M_HOSTAP &&
	    sc->sc_firmware_type == WI_INTERSIL) {
		/* XXX: some card need to be re-enabled for hostap */
		wi_cmd(sc, WI_CMD_DISABLE | WI_PORT0, 0, 0, 0);
		wi_cmd(sc, WI_CMD_ENABLE | WI_PORT0, 0, 0, 0);
	}

	if (ic->ic_opmode == IEEE80211_M_STA &&
	    ((ic->ic_flags & IEEE80211_F_DESBSSID) ||
	    ic->ic_des_chan != IEEE80211_CHAN_ANYC)) {
		memset(&join, 0, sizeof(join));
		if (ic->ic_flags & IEEE80211_F_DESBSSID)
			IEEE80211_ADDR_COPY(&join.wi_bssid, ic->ic_des_bssid);
		if (ic->ic_des_chan != IEEE80211_CHAN_ANYC)
			join.wi_chan =
			    htole16(ieee80211_chan2ieee(ic, ic->ic_des_chan));
		/* Lucent firmware does not support the JOIN RID. */
		if (sc->sc_firmware_type != WI_LUCENT)
			wi_write_rid(sc, WI_RID_JOIN_REQ, &join, sizeof(join));
	}

 out:
	if (error) {
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
		wi_stop(ifp, 0);
	}
	DPRINTF(("wi_init: return %d\n", error));
	return error;
}

static void
wi_stop(struct ifnet *ifp, int disable)
{
	struct wi_softc	*sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	if (!sc->sc_enabled)
		return;

	s = splnet();

	DPRINTF(("wi_stop: disable %d\n", disable));

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
	if (!sc->sc_invalid) {
		CSR_WRITE_2(sc, WI_INT_EN, 0);
		wi_cmd(sc, WI_CMD_DISABLE | sc->sc_portnum, 0, 0, 0);
	}

	wi_rssdescs_reset(ic, &sc->sc_rssd, &sc->sc_rssdfree,
	    &sc->sc_txpending);

	sc->sc_tx_timer = 0;
	sc->sc_scan_timer = 0;
	sc->sc_syn_timer = 0;
	sc->sc_false_syns = 0;
	sc->sc_naps = 0;
	ifp->if_flags &= ~(IFF_OACTIVE | IFF_RUNNING);
	ifp->if_timer = 0;

	if (disable) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}
	splx(s);
}

/*
 * Choose a data rate for a packet len bytes long that suits the packet
 * type and the wireless conditions.
 *
 * TBD Adapt fragmentation threshold.
 */
static void
wi_choose_rate(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_frame *wh, u_int len)
{
	struct wi_softc	*sc = ic->ic_if.if_softc;
	struct wi_node *wn = (void*)ni;
	struct ieee80211_rssadapt *ra = &wn->wn_rssadapt;
	int do_not_adapt, i, rateidx, s;

	do_not_adapt = (ic->ic_opmode != IEEE80211_M_HOSTAP) &&
	    (sc->sc_flags & WI_FLAGS_RSSADAPTSTA) == 0;

	s = splnet();

	rateidx = ieee80211_rssadapt_choose(ra, &ni->ni_rates, wh, len,
	    ic->ic_fixed_rate,
	    ((ic->ic_if.if_flags & IFF_DEBUG) == 0) ? NULL : ic->ic_if.if_xname,
	    do_not_adapt);

	if (ic->ic_opmode != IEEE80211_M_HOSTAP) {
		/* choose the slowest pending rate so that we don't
		 * accidentally send a packet on the MAC's queue
		 * too fast. TBD find out if the MAC labels Tx
		 * packets w/ rate when enqueued or dequeued.
		 */   
		for (i = 0; i < rateidx && sc->sc_txpending[i] == 0; i++);
		ni->ni_txrate = i;
	} else
		ni->ni_txrate = rateidx;
	splx(s);
	return;
}

static void
wi_raise_rate(struct ieee80211com *ic, struct ieee80211_rssdesc *id)
{
	struct wi_node *wn;
	if (id->id_node == NULL)
		return;

	wn = (void*)id->id_node;
	ieee80211_rssadapt_raise_rate(ic, &wn->wn_rssadapt, id);
}

static void
wi_lower_rate(struct ieee80211com *ic, struct ieee80211_rssdesc *id)
{
	struct ieee80211_node *ni;
	struct wi_node *wn;
	int s;

	s = splnet();

	if ((ni = id->id_node) == NULL) {
		DPRINTF(("wi_lower_rate: missing node\n"));
		goto out;
	}

	wn = (void *)ni;

	ieee80211_rssadapt_lower_rate(ic, ni, &wn->wn_rssadapt, id);
out:
	splx(s);
	return;
}

static void
wi_start(struct ifnet *ifp)
{
	struct wi_softc	*sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ieee80211_frame *wh;
	struct ieee80211_rateset *rs;
	struct wi_rssdesc *rd;
	struct ieee80211_rssdesc *id;
	struct mbuf *m0;
	struct wi_frame frmhdr;
	int cur, fid, off;

	if (!sc->sc_enabled || sc->sc_invalid)
		return;
	if (sc->sc_flags & WI_FLAGS_OUTRANGE)
		return;

	memset(&frmhdr, 0, sizeof(frmhdr));
	cur = sc->sc_txnext;
	for (;;) {
		ni = ic->ic_bss;
		if (!IF_IS_EMPTY(&ic->ic_mgtq)) {
			if (sc->sc_txd[cur].d_len != 0 ||
			    SLIST_EMPTY(&sc->sc_rssdfree)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IF_DEQUEUE(&ic->ic_mgtq, m0);
			m_copydata(m0, 4, ETHER_ADDR_LEN * 2,
			    (caddr_t)&frmhdr.wi_ehdr);
			frmhdr.wi_ehdr.ether_type = 0;
                        wh = mtod(m0, struct ieee80211_frame *);
			ni = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
			m0->m_pkthdr.rcvif = NULL;
		} else if (!IF_IS_EMPTY(&ic->ic_pwrsaveq)) {
			struct llc *llc;

			/* 
			 * Should these packets be processed after the
			 * regular packets or before?  Since they are being
			 * probed for, they are probably less time critical
			 * than other packets, but, on the other hand,
			 * we want the power saving nodes to go back to
			 * sleep as quickly as possible to save power...
			 */

			if (ic->ic_state != IEEE80211_S_RUN)
				break;

			if (sc->sc_txd[cur].d_len != 0 ||
			    SLIST_EMPTY(&sc->sc_rssdfree)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IF_DEQUEUE(&ic->ic_pwrsaveq, m0);
                        wh = mtod(m0, struct ieee80211_frame *);
			llc = (struct llc *) (wh + 1);
			m_copydata(m0, 4, ETHER_ADDR_LEN * 2,
			    (caddr_t)&frmhdr.wi_ehdr);
			frmhdr.wi_ehdr.ether_type = llc->llc_snap.ether_type;
			ni = (struct ieee80211_node *)m0->m_pkthdr.rcvif;
			m0->m_pkthdr.rcvif = NULL;
		} else {
			if (ic->ic_state != IEEE80211_S_RUN) {
				break;
			}
			IFQ_POLL(&ifp->if_snd, m0);
			if (m0 == NULL) {
				break;
			}
			if (sc->sc_txd[cur].d_len != 0 ||
			    SLIST_EMPTY(&sc->sc_rssdfree)) {
				ifp->if_flags |= IFF_OACTIVE;
				break;
			}
			IFQ_DEQUEUE(&ifp->if_snd, m0);
			ifp->if_opackets++;
			m_copydata(m0, 0, ETHER_HDR_LEN, 
			    (caddr_t)&frmhdr.wi_ehdr);
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m0);
#endif

			if ((m0 = ieee80211_encap(ifp, m0, &ni)) == NULL) {
				ifp->if_oerrors++;
				continue;
			}
                        wh = mtod(m0, struct ieee80211_frame *);
			if (ic->ic_flags & IEEE80211_F_WEPON)
				wh->i_fc[1] |= IEEE80211_FC1_WEP;
			if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
			    !IEEE80211_IS_MULTICAST(wh->i_addr1) &&
			    (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) ==
			    IEEE80211_FC0_TYPE_DATA) {
				if (ni->ni_associd == 0) {
					m_freem(m0);
					ifp->if_oerrors++;
					goto next;
				}
				if (ni->ni_pwrsave & IEEE80211_PS_SLEEP) {
					ieee80211_pwrsave(ic, ni, m0);
					continue; /* don't free node. */
				}
			}
		}
#if NBPFILTER > 0
		if (ic->ic_rawbpf)
			bpf_mtap(ic->ic_rawbpf, m0);
#endif
		frmhdr.wi_tx_ctl =
		    htole16(WI_ENC_TX_802_11|WI_TXCNTL_TX_EX|WI_TXCNTL_TX_OK);
		if (ic->ic_opmode == IEEE80211_M_HOSTAP)
			frmhdr.wi_tx_ctl |= htole16(WI_TXCNTL_ALTRTRY);
		if (ic->ic_opmode == IEEE80211_M_HOSTAP &&
		    (wh->i_fc[1] & IEEE80211_FC1_WEP)) {
			if ((m0 = ieee80211_wep_crypt(ifp, m0, 1)) == NULL) {
				ifp->if_oerrors++;
				goto next;
			}
			frmhdr.wi_tx_ctl |= htole16(WI_TXCNTL_NOCRYPT);
		}

		wi_choose_rate(ic, ni, wh, m0->m_pkthdr.len);

#if NBPFILTER > 0
		if (sc->sc_drvbpf) {
			struct mbuf mb;

			struct wi_tx_radiotap_header *tap = &sc->sc_txtap;

			tap->wt_rate = ni->ni_rates.rs_rates[ni->ni_txrate];
			tap->wt_chan_freq =
			    htole16(ic->ic_bss->ni_chan->ic_freq);
			tap->wt_chan_flags =
			    htole16(ic->ic_bss->ni_chan->ic_flags);

			/* TBD tap->wt_flags */

			M_COPY_PKTHDR(&mb, m0);
			mb.m_data = (caddr_t)tap;
			mb.m_len = tap->wt_ihdr.it_len;
			mb.m_next = m0;
			mb.m_pkthdr.len += mb.m_len;
			bpf_mtap(sc->sc_drvbpf, &mb);
		}
#endif
		rs = &ni->ni_rates;
		rd = SLIST_FIRST(&sc->sc_rssdfree);
		id = &rd->rd_desc;
		id->id_len = m0->m_pkthdr.len;
		sc->sc_txd[cur].d_rate = id->id_rateidx = ni->ni_txrate;
		id->id_rssi = ni->ni_rssi;

		frmhdr.wi_tx_idx = rd - sc->sc_rssd;

		if (ic->ic_opmode == IEEE80211_M_HOSTAP)
			frmhdr.wi_tx_rate = 5 * (rs->rs_rates[ni->ni_txrate] &
			    IEEE80211_RATE_VAL);
		else if (sc->sc_flags & WI_FLAGS_RSSADAPTSTA)
			(void)wi_write_txrate(sc, rs->rs_rates[ni->ni_txrate]);

		m_copydata(m0, 0, sizeof(struct ieee80211_frame),
		    (caddr_t)&frmhdr.wi_whdr);
		m_adj(m0, sizeof(struct ieee80211_frame));
		frmhdr.wi_dat_len = htole16(m0->m_pkthdr.len);
		if (IFF_DUMPPKTS(ifp))
			wi_dump_pkt(&frmhdr, ni, -1);
		fid = sc->sc_txd[cur].d_fid;
		off = sizeof(frmhdr);
		if (wi_write_bap(sc, fid, 0, &frmhdr, sizeof(frmhdr)) != 0 ||
		    wi_mwrite_bap(sc, fid, off, m0, m0->m_pkthdr.len) != 0) {
			ifp->if_oerrors++;
			m_freem(m0);
			goto next;
		}
		m_freem(m0);
		sc->sc_txd[cur].d_len = off;
		if (sc->sc_txcur == cur) {
			if (wi_cmd(sc, WI_CMD_TX | WI_RECLAIM, fid, 0, 0)) {
				printf("%s: xmit failed\n",
				    sc->sc_dev.dv_xname);
				sc->sc_txd[cur].d_len = 0;
				goto next;
			}
			sc->sc_txpending[ni->ni_txrate]++;
			sc->sc_tx_timer = 5;
			ifp->if_timer = 1;
		}
		sc->sc_txnext = cur = (cur + 1) % WI_NTXBUF;
		SLIST_REMOVE_HEAD(&sc->sc_rssdfree, rd_next);
		id->id_node = ni;
		continue;
next:
		if (ni != NULL && ni != ic->ic_bss)
			ieee80211_free_node(ic, ni);
	}
}


static int
wi_reset(struct wi_softc *sc)
{
	int i, error;

	DPRINTF(("wi_reset\n"));

	if (sc->sc_reset)
		(*sc->sc_reset)(sc);

	error = 0;
	for (i = 0; i < 5; i++) {
		DELAY(20*1000);	/* XXX: way too long! */
		if ((error = wi_cmd(sc, WI_CMD_INI, 0, 0, 0)) == 0)
			break;
	}
	if (error) {
		printf("%s: init failed\n", sc->sc_dev.dv_xname);
		return error;
	}
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, ~0);

	/* Calibrate timer. */
	wi_write_val(sc, WI_RID_TICK_TIME, 0);
	return 0;
}

static void
wi_watchdog(struct ifnet *ifp)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;

	ifp->if_timer = 0;
	if (!sc->sc_enabled)
		return;

	if (sc->sc_tx_timer) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", ifp->if_xname);
			ifp->if_oerrors++;
			wi_init(ifp);
			return;
		}
		ifp->if_timer = 1;
	}

	if (sc->sc_scan_timer) {
		if (--sc->sc_scan_timer <= WI_SCAN_WAIT - WI_SCAN_INQWAIT &&
		    sc->sc_firmware_type == WI_INTERSIL) {
			DPRINTF(("wi_watchdog: inquire scan\n"));
			wi_cmd(sc, WI_CMD_INQUIRE, WI_INFO_SCAN_RESULTS, 0, 0);
		}
		if (sc->sc_scan_timer)
			ifp->if_timer = 1;
	}

	if (sc->sc_syn_timer) {
		if (--sc->sc_syn_timer == 0) {
			DPRINTF2(("%s: %d false syns\n",
			    sc->sc_dev.dv_xname, sc->sc_false_syns));
			sc->sc_false_syns = 0;
			ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
			sc->sc_syn_timer = 5;
		}
		ifp->if_timer = 1;
	}

	/* TODO: rate control */
	ieee80211_watchdog(ifp);
}

static int
wi_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return ENXIO;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		/*
		 * Can't do promisc and hostap at the same time.  If all that's
		 * changing is the promisc flag, try to short-circuit a call to
		 * wi_init() by just setting PROMISC in the hardware.
		 */
		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_enabled) {
				if (ic->ic_opmode != IEEE80211_M_HOSTAP &&
				    (ifp->if_flags & IFF_PROMISC) != 0)
					wi_write_val(sc, WI_RID_PROMISC, 1);
				else
					wi_write_val(sc, WI_RID_PROMISC, 0);
			} else
				error = wi_init(ifp);
		} else if (sc->sc_enabled)
			wi_stop(ifp, 1);
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &ic->ic_media, cmd);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ic.ic_ec) :
		    ether_delmulti(ifr, &sc->sc_ic.ic_ec);
		if (error == ENETRESET) {
			if (sc->sc_enabled) {
				/* do not rescan */
				error = wi_write_multi(sc);
			} else
				error = 0;
		}
		break;
	case SIOCGIFGENERIC:
		error = wi_get_cfg(ifp, cmd, data);
		break;
	case SIOCSIFGENERIC:
		error = suser(curproc->p_ucred, &curproc->p_acflag);
		if (error)
			break;
		error = wi_set_cfg(ifp, cmd, data);
		if (error == ENETRESET) {
			if (sc->sc_enabled)
				error = wi_init(ifp);
			else
				error = 0;
		}
		break;
	case SIOCS80211BSSID:
		if (sc->sc_firmware_type == WI_LUCENT) {
			error = ENODEV;
			break;
		}
		/* fall through */
	default:
		error = ieee80211_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (sc->sc_enabled)
				error = wi_init(ifp);
			else
				error = 0;
		}
		break;
	}
	splx(s);
	return error;
}

/* TBD factor with ieee80211_media_change */
static int
wi_media_change(struct ifnet *ifp)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifmedia_entry *ime;
	enum ieee80211_opmode newmode;
	int i, rate, error = 0;

	ime = ic->ic_media.ifm_cur;
	if (IFM_SUBTYPE(ime->ifm_media) == IFM_AUTO) {
		i = -1;
	} else {
		struct ieee80211_rateset *rs =
		    &ic->ic_sup_rates[ieee80211_chan2mode(ic,
		        ic->ic_bss->ni_chan)];
		rate = ieee80211_media2rate(ime->ifm_media);
		if (rate == 0)
			return EINVAL;
		for (i = 0; i < rs->rs_nrates; i++) {
			if ((rs->rs_rates[i] & IEEE80211_RATE_VAL) == rate)
				break;
		}
		if (i == rs->rs_nrates)
			return EINVAL;
	}
	if (ic->ic_fixed_rate != i) {
		ic->ic_fixed_rate = i;
		error = ENETRESET;
	}

	if ((ime->ifm_media & IFM_IEEE80211_ADHOC) &&
	    (ime->ifm_media & IFM_FLAG0))
		newmode = IEEE80211_M_AHDEMO;
	else if (ime->ifm_media & IFM_IEEE80211_ADHOC)
		newmode = IEEE80211_M_IBSS;
	else if (ime->ifm_media & IFM_IEEE80211_HOSTAP)
		newmode = IEEE80211_M_HOSTAP;
	else if (ime->ifm_media & IFM_IEEE80211_MONITOR)
		newmode = IEEE80211_M_MONITOR;
	else
		newmode = IEEE80211_M_STA;
	if (ic->ic_opmode != newmode) {
		ic->ic_opmode = newmode;
		error = ENETRESET;
	}
	if (error == ENETRESET) {
		if (sc->sc_enabled)
			error = wi_init(ifp);
		else
			error = 0;
	}
	ifp->if_baudrate = ifmedia_baudrate(ic->ic_media.ifm_cur->ifm_media);

	return error;
}

static void
wi_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	u_int16_t val;
	int rate, len;

	if (sc->sc_enabled == 0) {
		imr->ifm_active = IFM_IEEE80211 | IFM_NONE;
		imr->ifm_status = 0;
		return;
	}

	imr->ifm_status = IFM_AVALID;
	imr->ifm_active = IFM_IEEE80211;
	if (ic->ic_state == IEEE80211_S_RUN &&
	    (sc->sc_flags & WI_FLAGS_OUTRANGE) == 0)
		imr->ifm_status |= IFM_ACTIVE;
	len = sizeof(val);
	if (wi_read_rid(sc, WI_RID_CUR_TX_RATE, &val, &len) != 0)
		rate = 0;
	else {
		/* convert to 802.11 rate */
		val = le16toh(val);
		rate = val * 2;
		if (sc->sc_firmware_type == WI_LUCENT) {
			if (rate == 10)
				rate = 11;	/* 5.5Mbps */
		} else {
			if (rate == 4*2)
				rate = 11;	/* 5.5Mbps */
			else if (rate == 8*2)
				rate = 22;	/* 11Mbps */
		}
	}
	imr->ifm_active |= ieee80211_rate2media(ic, rate, IEEE80211_MODE_11B);
	switch (ic->ic_opmode) {
	case IEEE80211_M_STA:
		break;
	case IEEE80211_M_IBSS:
		imr->ifm_active |= IFM_IEEE80211_ADHOC;
		break;
	case IEEE80211_M_AHDEMO:
		imr->ifm_active |= IFM_IEEE80211_ADHOC | IFM_FLAG0;
		break;
	case IEEE80211_M_HOSTAP:
		imr->ifm_active |= IFM_IEEE80211_HOSTAP;
		break;
	case IEEE80211_M_MONITOR:
		imr->ifm_active |= IFM_IEEE80211_MONITOR;
		break;
	}
}

static struct ieee80211_node *
wi_node_alloc(struct ieee80211com *ic)
{
	struct wi_node *wn =
	    malloc(sizeof(struct wi_node), M_DEVBUF, M_NOWAIT | M_ZERO);
	return wn ? &wn->wn_node : NULL;
}

static void
wi_node_free(struct ieee80211com *ic, struct ieee80211_node *ni)
{
	struct wi_softc *sc = ic->ic_if.if_softc;
	int i;

	for (i = 0; i < WI_NTXRSS; i++) {
		if (sc->sc_rssd[i].rd_desc.id_node == ni)
			sc->sc_rssd[i].rd_desc.id_node = NULL;
	}
	free(ni, M_DEVBUF);
}

static void
wi_node_copy(struct ieee80211com *ic, struct ieee80211_node *dst,
    const struct ieee80211_node *src)
{
	*(struct wi_node *)dst = *(const struct wi_node *)src;
}

static void
wi_sync_bssid(struct wi_softc *sc, u_int8_t new_bssid[IEEE80211_ADDR_LEN])
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct ifnet *ifp = &ic->ic_if;

	if (IEEE80211_ADDR_EQ(new_bssid, ni->ni_bssid))
		return;

	DPRINTF(("wi_sync_bssid: bssid %s -> ", ether_sprintf(ni->ni_bssid)));
	DPRINTF(("%s ?\n", ether_sprintf(new_bssid)));

	/* In promiscuous mode, the BSSID field is not a reliable
	 * indicator of the firmware's BSSID. Damp spurious
	 * change-of-BSSID indications.
	 */
	if ((ifp->if_flags & IFF_PROMISC) != 0 &&
	    sc->sc_false_syns >= WI_MAX_FALSE_SYNS)
		return;

	ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
}

static __inline void
wi_rssadapt_input(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_frame *wh, int rssi)
{
	struct wi_node *wn;

	if (ni == NULL) {
		printf("%s: null node", __func__);
		return;
	}

	wn = (void*)ni;
	ieee80211_rssadapt_input(ic, ni, &wn->wn_rssadapt, rssi);
}

static void
wi_rx_intr(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_node *ni;
	struct wi_frame frmhdr;
	struct mbuf *m;
	struct ieee80211_frame *wh;
	int fid, len, off, rssi;
	u_int8_t dir;
	u_int16_t status;
	u_int32_t rstamp;

	fid = CSR_READ_2(sc, WI_RX_FID);

	/* First read in the frame header */
	if (wi_read_bap(sc, fid, 0, &frmhdr, sizeof(frmhdr))) {
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
		ifp->if_ierrors++;
		DPRINTF(("wi_rx_intr: read fid %x failed\n", fid));
		return;
	}

	if (IFF_DUMPPKTS(ifp))
		wi_dump_pkt(&frmhdr, NULL, frmhdr.wi_rx_signal);

	/*
	 * Drop undecryptable or packets with receive errors here
	 */
	status = le16toh(frmhdr.wi_status);
	if ((status & WI_STAT_ERRSTAT) != 0 &&
	    ic->ic_opmode != IEEE80211_M_MONITOR) {
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
		ifp->if_ierrors++;
		DPRINTF(("wi_rx_intr: fid %x error status %x\n", fid, status));
		return;
	}
	rssi = frmhdr.wi_rx_signal;
	rstamp = (le16toh(frmhdr.wi_rx_tstamp0) << 16) |
	    le16toh(frmhdr.wi_rx_tstamp1);

	len = le16toh(frmhdr.wi_dat_len);
	off = ALIGN(sizeof(struct ieee80211_frame));

	/* Sometimes the PRISM2.x returns bogusly large frames. Except
	 * in monitor mode, just throw them away.
	 */
	if (off + len > MCLBYTES) {
		if (ic->ic_opmode != IEEE80211_M_MONITOR) {
			CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
			ifp->if_ierrors++;
			DPRINTF(("wi_rx_intr: oversized packet\n"));
			return;
		} else
			len = 0;
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
		ifp->if_ierrors++;
		DPRINTF(("wi_rx_intr: MGET failed\n"));
		return;
	}
	if (off + len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
			m_freem(m);
			ifp->if_ierrors++;
			DPRINTF(("wi_rx_intr: MCLGET failed\n"));
			return;
		}
	}

	m->m_data += off - sizeof(struct ieee80211_frame);
	memcpy(m->m_data, &frmhdr.wi_whdr, sizeof(struct ieee80211_frame));
	wi_read_bap(sc, fid, sizeof(frmhdr),
	    m->m_data + sizeof(struct ieee80211_frame), len);
	m->m_pkthdr.len = m->m_len = sizeof(struct ieee80211_frame) + len;
	m->m_pkthdr.rcvif = ifp;

	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);

#if NBPFILTER > 0
	if (sc->sc_drvbpf) {
		struct mbuf mb;
		struct wi_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_rate = frmhdr.wi_rx_rate / 5;
		tap->wr_antsignal = WI_RSSI_TO_DBM(sc, frmhdr.wi_rx_signal);
		tap->wr_antnoise = WI_RSSI_TO_DBM(sc, frmhdr.wi_rx_silence);

		tap->wr_chan_freq = htole16(ic->ic_bss->ni_chan->ic_freq);
		tap->wr_chan_flags = htole16(ic->ic_bss->ni_chan->ic_flags);
		if (frmhdr.wi_status & WI_STAT_PCF)
			tap->wr_flags |= IEEE80211_RADIOTAP_F_CFP;

		M_COPY_PKTHDR(&mb, m);
		mb.m_data = (caddr_t)tap;
		mb.m_len = tap->wr_ihdr.it_len;
		mb.m_next = m;
		mb.m_pkthdr.len += mb.m_len;
		bpf_mtap(sc->sc_drvbpf, &mb);
	}
#endif
	wh = mtod(m, struct ieee80211_frame *);
	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		/*
		 * WEP is decrypted by hardware. Clear WEP bit
		 * header for ieee80211_input().
		 */
		wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
	}

	/* synchronize driver's BSSID with firmware's BSSID */
	dir = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;
	if (ic->ic_opmode == IEEE80211_M_IBSS && dir == IEEE80211_FC1_DIR_NODS)
		wi_sync_bssid(sc, wh->i_addr3);

	ni = ieee80211_find_rxnode(ic, wh);

	ieee80211_input(ifp, m, ni, rssi, rstamp);

	wi_rssadapt_input(ic, ni, wh, rssi);

	/*
	 * The frame may have caused the node to be marked for
	 * reclamation (e.g. in response to a DEAUTH message)
	 * so use free_node here instead of unref_node.
	 */
	if (ni == ic->ic_bss)
		ieee80211_unref_node(&ni);
	else
		ieee80211_free_node(ic, ni);
}

static void
wi_tx_ex_intr(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_node *ni;
	struct ieee80211_rssdesc *id;
	struct wi_rssdesc *rssd;
	struct wi_frame frmhdr;
	int fid;
	u_int16_t status;

	fid = CSR_READ_2(sc, WI_TX_CMP_FID);
	/* Read in the frame header */
	if (wi_read_bap(sc, fid, 0, &frmhdr, sizeof(frmhdr)) != 0) {
		printf("%s: %s read fid %x failed\n", sc->sc_dev.dv_xname,
		    __func__, fid);
		wi_rssdescs_reset(ic, &sc->sc_rssd, &sc->sc_rssdfree,
		    &sc->sc_txpending);
		goto out;
	}

	if (frmhdr.wi_tx_idx >= WI_NTXRSS) {
		printf("%s: %s bad idx %02x\n",
		    sc->sc_dev.dv_xname, __func__, frmhdr.wi_tx_idx);
		wi_rssdescs_reset(ic, &sc->sc_rssd, &sc->sc_rssdfree,
		    &sc->sc_txpending);
		goto out;
	}

	status = le16toh(frmhdr.wi_status);

	/*
	 * Spontaneous station disconnects appear as xmit
	 * errors.  Don't announce them and/or count them
	 * as an output error.
	 */
	if (ppsratecheck(&lasttxerror, &curtxeps, wi_txerate)) {
		printf("%s: tx failed", sc->sc_dev.dv_xname);
		if (status & WI_TXSTAT_RET_ERR)
			printf(", retry limit exceeded");
		if (status & WI_TXSTAT_AGED_ERR)
			printf(", max transmit lifetime exceeded");
		if (status & WI_TXSTAT_DISCONNECT)
			printf(", port disconnected");
		if (status & WI_TXSTAT_FORM_ERR)
			printf(", invalid format (data len %u src %s)",
				le16toh(frmhdr.wi_dat_len),
				ether_sprintf(frmhdr.wi_ehdr.ether_shost));
		if (status & ~0xf)
			printf(", status=0x%x", status);
		printf("\n");
	}
	ifp->if_oerrors++;
	rssd = &sc->sc_rssd[frmhdr.wi_tx_idx];
	id = &rssd->rd_desc;
	if ((status & WI_TXSTAT_RET_ERR) != 0)
		wi_lower_rate(ic, id);

	ni = id->id_node;
	id->id_node = NULL;

	if (ni == NULL) {
		printf("%s: %s null node, rssdesc %02x\n",
		    sc->sc_dev.dv_xname, __func__, frmhdr.wi_tx_idx);
		goto out;
	}

	if (sc->sc_txpending[id->id_rateidx]-- == 0) {
	        printf("%s: %s txpending[%i] wraparound", sc->sc_dev.dv_xname,
		    __func__, id->id_rateidx);
		sc->sc_txpending[id->id_rateidx] = 0;
	}
	if (ni != NULL && ni != ic->ic_bss)
		ieee80211_free_node(ic, ni);
	SLIST_INSERT_HEAD(&sc->sc_rssdfree, rssd, rd_next);
out:
	ifp->if_flags &= ~IFF_OACTIVE;
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_TX_EXC);
}

static void
wi_txalloc_intr(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int fid, cur;

	fid = CSR_READ_2(sc, WI_ALLOC_FID);
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_ALLOC);

	cur = sc->sc_txcur;
	if (sc->sc_txd[cur].d_fid != fid) {
		printf("%s: bad alloc %x != %x, cur %d nxt %d\n",
		    sc->sc_dev.dv_xname, fid, sc->sc_txd[cur].d_fid, cur,
		    sc->sc_txnext);
		return;
	}
	sc->sc_tx_timer = 0;
	sc->sc_txd[cur].d_len = 0;
	sc->sc_txcur = cur = (cur + 1) % WI_NTXBUF;
	if (sc->sc_txd[cur].d_len == 0)
		ifp->if_flags &= ~IFF_OACTIVE;
	else {
		if (wi_cmd(sc, WI_CMD_TX | WI_RECLAIM, sc->sc_txd[cur].d_fid,
		    0, 0)) {
			printf("%s: xmit failed\n", sc->sc_dev.dv_xname);
			sc->sc_txd[cur].d_len = 0;
		} else {
			sc->sc_txpending[sc->sc_txd[cur].d_rate]++;
			sc->sc_tx_timer = 5;
			ifp->if_timer = 1;
		}
	}
}

static void
wi_tx_intr(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	struct ieee80211_node *ni;
	struct ieee80211_rssdesc *id;
	struct wi_rssdesc *rssd;
	struct wi_frame frmhdr;
	int fid;

	fid = CSR_READ_2(sc, WI_TX_CMP_FID);
	/* Read in the frame header */
	if (wi_read_bap(sc, fid, 0, &frmhdr, sizeof(frmhdr)) != 0) {
		printf("%s: %s read fid %x failed\n", sc->sc_dev.dv_xname,
		    __func__, fid);
		wi_rssdescs_reset(ic, &sc->sc_rssd, &sc->sc_rssdfree,
		    &sc->sc_txpending);
		goto out;
	}

	if (frmhdr.wi_tx_idx >= WI_NTXRSS) {
		printf("%s: %s bad idx %02x\n",
		    sc->sc_dev.dv_xname, __func__, frmhdr.wi_tx_idx);
		wi_rssdescs_reset(ic, &sc->sc_rssd, &sc->sc_rssdfree,
		    &sc->sc_txpending);
		goto out;
	}

	rssd = &sc->sc_rssd[frmhdr.wi_tx_idx];
	id = &rssd->rd_desc;
	wi_raise_rate(ic, id);

	ni = id->id_node;
	id->id_node = NULL;

	if (ni == NULL) {
		printf("%s: %s null node, rssdesc %02x\n",
		    sc->sc_dev.dv_xname, __func__, frmhdr.wi_tx_idx);
		goto out;
	}

	if (sc->sc_txpending[id->id_rateidx]-- == 0) {
	        printf("%s: %s txpending[%i] wraparound", sc->sc_dev.dv_xname,
		    __func__, id->id_rateidx);
		sc->sc_txpending[id->id_rateidx] = 0;
	}
	if (ni != NULL && ni != ic->ic_bss)
		ieee80211_free_node(ic, ni);
	SLIST_INSERT_HEAD(&sc->sc_rssdfree, rssd, rd_next);
out:
	ifp->if_flags &= ~IFF_OACTIVE;
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_TX);
}

static void
wi_info_intr(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int i, fid, len, off;
	u_int16_t ltbuf[2];
	u_int16_t stat;
	u_int32_t *ptr;

	fid = CSR_READ_2(sc, WI_INFO_FID);
	wi_read_bap(sc, fid, 0, ltbuf, sizeof(ltbuf));

	switch (le16toh(ltbuf[1])) {

	case WI_INFO_LINK_STAT:
		wi_read_bap(sc, fid, sizeof(ltbuf), &stat, sizeof(stat));
		DPRINTF(("wi_info_intr: LINK_STAT 0x%x\n", le16toh(stat)));
		switch (le16toh(stat)) {
		case CONNECTED:
			sc->sc_flags &= ~WI_FLAGS_OUTRANGE;
			if (ic->ic_state == IEEE80211_S_RUN &&
			    ic->ic_opmode != IEEE80211_M_IBSS)
				break;
			/* FALLTHROUGH */
		case AP_CHANGE:
			ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
			break;
		case AP_IN_RANGE:
			sc->sc_flags &= ~WI_FLAGS_OUTRANGE;
			break;
		case AP_OUT_OF_RANGE:
			if (sc->sc_firmware_type == WI_SYMBOL &&
			    sc->sc_scan_timer > 0) {
				if (wi_cmd(sc, WI_CMD_INQUIRE,
				    WI_INFO_HOST_SCAN_RESULTS, 0, 0) != 0)
					sc->sc_scan_timer = 0;
				break;
			}
			if (ic->ic_opmode == IEEE80211_M_STA)
				sc->sc_flags |= WI_FLAGS_OUTRANGE;
			break;
		case DISCONNECTED:
		case ASSOC_FAILED:
			if (ic->ic_opmode == IEEE80211_M_STA)
				ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
			break;
		}
		break;

	case WI_INFO_COUNTERS:
		/* some card versions have a larger stats structure */
		len = min(le16toh(ltbuf[0]) - 1, sizeof(sc->sc_stats) / 4);
		ptr = (u_int32_t *)&sc->sc_stats;
		off = sizeof(ltbuf);
		for (i = 0; i < len; i++, off += 2, ptr++) {
			wi_read_bap(sc, fid, off, &stat, sizeof(stat));
			stat = le16toh(stat);
#ifdef WI_HERMES_STATS_WAR
			if (stat & 0xf000)
				stat = ~stat;
#endif
			*ptr += stat;
		}
		ifp->if_collisions = sc->sc_stats.wi_tx_single_retries +
		    sc->sc_stats.wi_tx_multi_retries +
		    sc->sc_stats.wi_tx_retry_limit;
		break;

	case WI_INFO_SCAN_RESULTS:
	case WI_INFO_HOST_SCAN_RESULTS:
		wi_scan_result(sc, fid, le16toh(ltbuf[0]));
		break;

	default:
		DPRINTF(("wi_info_intr: got fid %x type %x len %d\n", fid,
		    le16toh(ltbuf[1]), le16toh(ltbuf[0])));
		break;
	}
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_INFO);
}

static int
wi_write_multi(struct wi_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int n;
	struct wi_mcast mlist;
	struct ether_multi *enm;
	struct ether_multistep estep;

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		memset(&mlist, 0, sizeof(mlist));
		return wi_write_rid(sc, WI_RID_MCAST_LIST, &mlist,
		    sizeof(mlist));
	}

	n = 0;
	ETHER_FIRST_MULTI(estep, &sc->sc_ic.ic_ec, enm);
	while (enm != NULL) {
		/* Punt on ranges or too many multicast addresses. */
		if (!IEEE80211_ADDR_EQ(enm->enm_addrlo, enm->enm_addrhi) ||
		    n >= sizeof(mlist) / sizeof(mlist.wi_mcast[0]))
			goto allmulti;

		IEEE80211_ADDR_COPY(&mlist.wi_mcast[n], enm->enm_addrlo);
		n++;
		ETHER_NEXT_MULTI(estep, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	return wi_write_rid(sc, WI_RID_MCAST_LIST, &mlist,
	    IEEE80211_ADDR_LEN * n);
}


static void
wi_read_nicid(struct wi_softc *sc)
{
	struct wi_card_ident *id;
	char *p;
	int len;
	u_int16_t ver[4];

	/* getting chip identity */
	memset(ver, 0, sizeof(ver));
	len = sizeof(ver);
	wi_read_rid(sc, WI_RID_CARD_ID, ver, &len);
	printf("%s: using ", sc->sc_dev.dv_xname);
DPRINTF2(("wi_read_nicid: CARD_ID: %x %x %x %x\n", le16toh(ver[0]), le16toh(ver[1]), le16toh(ver[2]), le16toh(ver[3])));

	sc->sc_firmware_type = WI_NOTYPE;
	for (id = wi_card_ident; id->card_name != NULL; id++) {
		if (le16toh(ver[0]) == id->card_id) {
			printf("%s", id->card_name);
			sc->sc_firmware_type = id->firm_type;
			break;
		}
	}
	if (sc->sc_firmware_type == WI_NOTYPE) {
		if (le16toh(ver[0]) & 0x8000) {
			printf("Unknown PRISM2 chip");
			sc->sc_firmware_type = WI_INTERSIL;
		} else {
			printf("Unknown Lucent chip");
			sc->sc_firmware_type = WI_LUCENT;
		}
	}

	/* get primary firmware version (Only Prism chips) */
	if (sc->sc_firmware_type != WI_LUCENT) {
		memset(ver, 0, sizeof(ver));
		len = sizeof(ver);
		wi_read_rid(sc, WI_RID_PRI_IDENTITY, ver, &len);
		sc->sc_pri_firmware_ver = le16toh(ver[2]) * 10000 +
		    le16toh(ver[3]) * 100 + le16toh(ver[1]);
	}

	/* get station firmware version */
	memset(ver, 0, sizeof(ver));
	len = sizeof(ver);
	wi_read_rid(sc, WI_RID_STA_IDENTITY, ver, &len);
	sc->sc_sta_firmware_ver = le16toh(ver[2]) * 10000 +
	    le16toh(ver[3]) * 100 + le16toh(ver[1]);
	if (sc->sc_firmware_type == WI_INTERSIL &&
	    (sc->sc_sta_firmware_ver == 10102 ||
	     sc->sc_sta_firmware_ver == 20102)) {
		char ident[12];
		memset(ident, 0, sizeof(ident));
		len = sizeof(ident);
		/* value should be the format like "V2.00-11" */
		if (wi_read_rid(sc, WI_RID_SYMBOL_IDENTITY, ident, &len) == 0 &&
		    *(p = (char *)ident) >= 'A' &&
		    p[2] == '.' && p[5] == '-' && p[8] == '\0') {
			sc->sc_firmware_type = WI_SYMBOL;
			sc->sc_sta_firmware_ver = (p[1] - '0') * 10000 +
			    (p[3] - '0') * 1000 + (p[4] - '0') * 100 +
			    (p[6] - '0') * 10 + (p[7] - '0');
		}
	}

	printf("\n%s: %s Firmware: ", sc->sc_dev.dv_xname,
	     sc->sc_firmware_type == WI_LUCENT ? "Lucent" :
	    (sc->sc_firmware_type == WI_SYMBOL ? "Symbol" : "Intersil"));
	if (sc->sc_firmware_type != WI_LUCENT)	/* XXX */
		printf("Primary (%u.%u.%u), ",
		    sc->sc_pri_firmware_ver / 10000,
		    (sc->sc_pri_firmware_ver % 10000) / 100,
		    sc->sc_pri_firmware_ver % 100);
	printf("Station (%u.%u.%u)\n",
	    sc->sc_sta_firmware_ver / 10000,
	    (sc->sc_sta_firmware_ver % 10000) / 100,
	    sc->sc_sta_firmware_ver % 100);
}

static int
wi_write_ssid(struct wi_softc *sc, int rid, u_int8_t *buf, int buflen)
{
	struct wi_ssid ssid;

	if (buflen > IEEE80211_NWID_LEN)
		return ENOBUFS;
	memset(&ssid, 0, sizeof(ssid));
	ssid.wi_len = htole16(buflen);
	memcpy(ssid.wi_ssid, buf, buflen);
	return wi_write_rid(sc, rid, &ssid, sizeof(ssid));
}

static int
wi_get_cfg(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifreq *ifr = (struct ifreq *)data;
	struct wi_req wreq;
	int len, n, error;

	error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
	if (error)
		return error;
	len = (wreq.wi_len - 1) * 2;
	if (len < sizeof(u_int16_t))
		return ENOSPC;
	if (len > sizeof(wreq.wi_val))
		len = sizeof(wreq.wi_val);

	switch (wreq.wi_type) {

	case WI_RID_IFACE_STATS:
		memcpy(wreq.wi_val, &sc->sc_stats, sizeof(sc->sc_stats));
		if (len < sizeof(sc->sc_stats))
			error = ENOSPC;
		else
			len = sizeof(sc->sc_stats);
		break;

	case WI_RID_ENCRYPTION:
	case WI_RID_TX_CRYPT_KEY:
	case WI_RID_DEFLT_CRYPT_KEYS:
	case WI_RID_TX_RATE:
		return ieee80211_cfgget(ifp, cmd, data);

	case WI_RID_MICROWAVE_OVEN:
		if (sc->sc_enabled && (sc->sc_flags & WI_FLAGS_HAS_MOR)) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		wreq.wi_val[0] = htole16(sc->sc_microwave_oven);
		len = sizeof(u_int16_t);
		break;

	case WI_RID_DBM_ADJUST:
		if (sc->sc_enabled && (sc->sc_flags & WI_FLAGS_HAS_DBMADJUST)) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		wreq.wi_val[0] = htole16(sc->sc_dbm_offset);
		len = sizeof(u_int16_t);
		break;

	case WI_RID_ROAMING_MODE:
		if (sc->sc_enabled && (sc->sc_flags & WI_FLAGS_HAS_ROAMING)) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		wreq.wi_val[0] = htole16(sc->sc_roaming_mode);
		len = sizeof(u_int16_t);
		break;

	case WI_RID_SYSTEM_SCALE:
		if (sc->sc_enabled && (sc->sc_flags & WI_FLAGS_HAS_SYSSCALE)) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		wreq.wi_val[0] = htole16(sc->sc_system_scale);
		len = sizeof(u_int16_t);
		break;

	case WI_RID_FRAG_THRESH:
		if (sc->sc_enabled && (sc->sc_flags & WI_FLAGS_HAS_FRAGTHR)) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		wreq.wi_val[0] = htole16(sc->sc_frag_thresh);
		len = sizeof(u_int16_t);
		break;

	case WI_RID_READ_APS:
		if (ic->ic_opmode == IEEE80211_M_HOSTAP)
			return ieee80211_cfgget(ifp, cmd, data);
		if (sc->sc_scan_timer > 0) {
			error = EINPROGRESS;
			break;
		}
		n = sc->sc_naps;
		if (len < sizeof(n)) {
			error = ENOSPC;
			break;
		}
		if (len < sizeof(n) + sizeof(struct wi_apinfo) * n)
			n = (len - sizeof(n)) / sizeof(struct wi_apinfo);
		len = sizeof(n) + sizeof(struct wi_apinfo) * n;
		memcpy(wreq.wi_val, &n, sizeof(n));
		memcpy((caddr_t)wreq.wi_val + sizeof(n), sc->sc_aps,
		    sizeof(struct wi_apinfo) * n);
		break;

	default:
		if (sc->sc_enabled) {
			error = wi_read_rid(sc, wreq.wi_type, wreq.wi_val,
			    &len);
			break;
		}
		switch (wreq.wi_type) {
		case WI_RID_MAX_DATALEN:
			wreq.wi_val[0] = htole16(sc->sc_max_datalen);
			len = sizeof(u_int16_t);
			break;
		case WI_RID_FRAG_THRESH:
			wreq.wi_val[0] = htole16(sc->sc_frag_thresh);
			len = sizeof(u_int16_t);
			break;
		case WI_RID_RTS_THRESH:
			wreq.wi_val[0] = htole16(sc->sc_rts_thresh);
			len = sizeof(u_int16_t);
			break;
		case WI_RID_CNFAUTHMODE:
			wreq.wi_val[0] = htole16(sc->sc_cnfauthmode);
			len = sizeof(u_int16_t);
			break;
		case WI_RID_NODENAME:
			if (len < sc->sc_nodelen + sizeof(u_int16_t)) {
				error = ENOSPC;
				break;
			}
			len = sc->sc_nodelen + sizeof(u_int16_t);
			wreq.wi_val[0] = htole16((sc->sc_nodelen + 1) / 2);
			memcpy(&wreq.wi_val[1], sc->sc_nodename,
			    sc->sc_nodelen);
			break;
		default:
			return ieee80211_cfgget(ifp, cmd, data);
		}
		break;
	}
	if (error)
		return error;
	wreq.wi_len = (len + 1) / 2 + 1;
	return copyout(&wreq, ifr->ifr_data, (wreq.wi_len + 1) * 2);
}

static int
wi_set_cfg(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct wi_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ieee80211_rateset *rs = &ic->ic_sup_rates[IEEE80211_MODE_11B];
	struct wi_req wreq;
	struct mbuf *m;
	int i, len, error;

	error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
	if (error)
		return error;
	len = (wreq.wi_len - 1) * 2;
	switch (wreq.wi_type) {
	case WI_RID_DBM_ADJUST:
		return ENODEV;

	case WI_RID_NODENAME:
		if (le16toh(wreq.wi_val[0]) * 2 > len ||
		    le16toh(wreq.wi_val[0]) > sizeof(sc->sc_nodename)) {
			error = ENOSPC;
			break;
		}
		if (sc->sc_enabled) {
			error = wi_write_rid(sc, wreq.wi_type, wreq.wi_val,
			    len);
			if (error)
				break;
		}
		sc->sc_nodelen = le16toh(wreq.wi_val[0]) * 2;
		memcpy(sc->sc_nodename, &wreq.wi_val[1], sc->sc_nodelen);
		break;

	case WI_RID_MICROWAVE_OVEN:
	case WI_RID_ROAMING_MODE:
	case WI_RID_SYSTEM_SCALE:
	case WI_RID_FRAG_THRESH:
		if (wreq.wi_type == WI_RID_MICROWAVE_OVEN &&
		    (sc->sc_flags & WI_FLAGS_HAS_MOR) == 0)
			break;
		if (wreq.wi_type == WI_RID_ROAMING_MODE &&
		    (sc->sc_flags & WI_FLAGS_HAS_ROAMING) == 0)
			break;
		if (wreq.wi_type == WI_RID_SYSTEM_SCALE &&
		    (sc->sc_flags & WI_FLAGS_HAS_SYSSCALE) == 0)
			break;
		if (wreq.wi_type == WI_RID_FRAG_THRESH &&
		    (sc->sc_flags & WI_FLAGS_HAS_FRAGTHR) == 0)
			break;
		/* FALLTHROUGH */
	case WI_RID_RTS_THRESH:
	case WI_RID_CNFAUTHMODE:
	case WI_RID_MAX_DATALEN:
		if (sc->sc_enabled) {
			error = wi_write_rid(sc, wreq.wi_type, wreq.wi_val,
			    sizeof(u_int16_t));
			if (error)
				break;
		}
		switch (wreq.wi_type) {
		case WI_RID_FRAG_THRESH:
			sc->sc_frag_thresh = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_RTS_THRESH:
			sc->sc_rts_thresh = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_MICROWAVE_OVEN:
			sc->sc_microwave_oven = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_ROAMING_MODE:
			sc->sc_roaming_mode = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_SYSTEM_SCALE:
			sc->sc_system_scale = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_CNFAUTHMODE:
			sc->sc_cnfauthmode = le16toh(wreq.wi_val[0]);
			break;
		case WI_RID_MAX_DATALEN:
			sc->sc_max_datalen = le16toh(wreq.wi_val[0]);
			break;
		}
		break;

	case WI_RID_TX_RATE:
		switch (le16toh(wreq.wi_val[0])) {
		case 3:
			ic->ic_fixed_rate = -1;
			break;
		default:
			for (i = 0; i < IEEE80211_RATE_SIZE; i++) {
				if ((rs->rs_rates[i] & IEEE80211_RATE_VAL)
				    / 2 == le16toh(wreq.wi_val[0]))
					break;
			}
			if (i == IEEE80211_RATE_SIZE)
				return EINVAL;
			ic->ic_fixed_rate = i;
		}
		if (sc->sc_enabled)
			error = wi_cfg_txrate(sc);
		break;

	case WI_RID_SCAN_APS:
		if (sc->sc_enabled && ic->ic_opmode != IEEE80211_M_HOSTAP)
			error = wi_scan_ap(sc, 0x3fff, 0x000f);
		break;

	case WI_RID_MGMT_XMIT:
		if (!sc->sc_enabled) {
			error = ENETDOWN;
			break;
		}
		if (ic->ic_mgtq.ifq_len > 5) {
			error = EAGAIN;
			break;
		}
		/* XXX wi_len looks in u_int8_t, not in u_int16_t */
		m = m_devget((char *)&wreq.wi_val, wreq.wi_len, 0, ifp, NULL);
		if (m == NULL) {
			error = ENOMEM;
			break;
		}
		IF_ENQUEUE(&ic->ic_mgtq, m);
		break;

	default:
		if (sc->sc_enabled) {
			error = wi_write_rid(sc, wreq.wi_type, wreq.wi_val,
			    len);
			if (error)
				break;
		}
		error = ieee80211_cfgset(ifp, cmd, data);
		break;
	}
	return error;
}

/* Rate is 0 for hardware auto-select, otherwise rate is
 * 2, 4, 11, or 22 (units of 500Kbps).
 */
static int
wi_write_txrate(struct wi_softc *sc, int rate)
{
	u_int16_t hwrate;
	int i;

	rate = (rate & IEEE80211_RATE_VAL) / 2;

	/* rate: 0, 1, 2, 5, 11 */
	switch (sc->sc_firmware_type) {
	case WI_LUCENT:
		switch (rate) {
		case 0:
			hwrate = 3;	/* auto */
			break;
		case 5:
			hwrate = 4;
			break;
		case 11:
			hwrate = 5;
			break;
		default:
			hwrate = rate;
			break;
		}
		break;
	default:
		/* Choose a bit according to this table.
		 *
		 * bit | data rate
		 * ----+-------------------
		 * 0   | 1Mbps
		 * 1   | 2Mbps
		 * 2   | 5.5Mbps
		 * 3   | 11Mbps
		 */
		for (i = 8; i > 0; i >>= 1) {
			if (rate >= i)
				break;
		}
		if (i == 0)
			hwrate = 0xf;	/* auto */
		else
			hwrate = i;
		break;
	}

	if (sc->sc_tx_rate == hwrate)
		return 0;

	if (sc->sc_if.if_flags & IFF_DEBUG)
		printf("%s: tx rate %d -> %d (%d)\n", __func__, sc->sc_tx_rate,
		    hwrate, rate);

	sc->sc_tx_rate = hwrate;

	return wi_write_val(sc, WI_RID_TX_RATE, sc->sc_tx_rate);
}

static int
wi_cfg_txrate(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_rateset *rs;
	int rate;

	rs = &ic->ic_sup_rates[IEEE80211_MODE_11B];

	sc->sc_tx_rate = 0; /* force write to RID */

	if (ic->ic_fixed_rate < 0)
		rate = 0;	/* auto */
	else
		rate = rs->rs_rates[ic->ic_fixed_rate];

	return wi_write_txrate(sc, rate);
}

static int
wi_write_wep(struct wi_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int error = 0;
	int i, keylen;
	u_int16_t val;
	struct wi_key wkey[IEEE80211_WEP_NKID];

	switch (sc->sc_firmware_type) {
	case WI_LUCENT:
		val = (ic->ic_flags & IEEE80211_F_WEPON) ? 1 : 0;
		error = wi_write_val(sc, WI_RID_ENCRYPTION, val);
		if (error)
			break;
		error = wi_write_val(sc, WI_RID_TX_CRYPT_KEY, ic->ic_wep_txkey);
		if (error)
			break;
		memset(wkey, 0, sizeof(wkey));
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			keylen = ic->ic_nw_keys[i].wk_len;
			wkey[i].wi_keylen = htole16(keylen);
			memcpy(wkey[i].wi_keydat, ic->ic_nw_keys[i].wk_key,
			    keylen);
		}
		error = wi_write_rid(sc, WI_RID_DEFLT_CRYPT_KEYS,
		    wkey, sizeof(wkey));
		break;

	case WI_INTERSIL:
	case WI_SYMBOL:
		if (ic->ic_flags & IEEE80211_F_WEPON) {
			/*
			 * ONLY HWB3163 EVAL-CARD Firmware version
			 * less than 0.8 variant2
			 *
			 *   If promiscuous mode disable, Prism2 chip
			 *  does not work with WEP .
			 * It is under investigation for details.
			 * (ichiro@NetBSD.org)
			 */
			if (sc->sc_firmware_type == WI_INTERSIL &&
			    sc->sc_sta_firmware_ver < 802 ) {
				/* firm ver < 0.8 variant 2 */
				wi_write_val(sc, WI_RID_PROMISC, 1);
			}
			wi_write_val(sc, WI_RID_CNFAUTHMODE,
			    sc->sc_cnfauthmode);
			val = PRIVACY_INVOKED | EXCLUDE_UNENCRYPTED;
			/*
			 * Encryption firmware has a bug for HostAP mode.
			 */
			if (sc->sc_firmware_type == WI_INTERSIL &&
			    ic->ic_opmode == IEEE80211_M_HOSTAP)
				val |= HOST_ENCRYPT;
		} else {
			wi_write_val(sc, WI_RID_CNFAUTHMODE,
			    IEEE80211_AUTH_OPEN);
			val = HOST_ENCRYPT | HOST_DECRYPT;
		}
		error = wi_write_val(sc, WI_RID_P2_ENCRYPTION, val);
		if (error)
			break;
		error = wi_write_val(sc, WI_RID_P2_TX_CRYPT_KEY,
		    ic->ic_wep_txkey);
		if (error)
			break;
		/*
		 * It seems that the firmware accept 104bit key only if
		 * all the keys have 104bit length.  We get the length of
		 * the transmit key and use it for all other keys.
		 * Perhaps we should use software WEP for such situation.
		 */
		keylen = ic->ic_nw_keys[ic->ic_wep_txkey].wk_len;
		if (keylen > IEEE80211_WEP_KEYLEN)
			keylen = 13;	/* 104bit keys */
		else
			keylen = IEEE80211_WEP_KEYLEN;
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			error = wi_write_rid(sc, WI_RID_P2_CRYPT_KEY0 + i,
			    ic->ic_nw_keys[i].wk_key, keylen);
			if (error)
				break;
		}
		break;
	}
	return error;
}

/* Must be called at proper protection level! */
static int
wi_cmd(struct wi_softc *sc, int cmd, int val0, int val1, int val2)
{
	int i, status;

	/* wait for the busy bit to clear */
	for (i = 500; i > 0; i--) {	/* 5s */
		if ((CSR_READ_2(sc, WI_COMMAND) & WI_CMD_BUSY) == 0)
			break;
		DELAY(10*1000);	/* 10 m sec */
	}
	if (i == 0) {
		printf("%s: wi_cmd: busy bit won't clear.\n",
		    sc->sc_dev.dv_xname);
		return(ETIMEDOUT);
  	}
	CSR_WRITE_2(sc, WI_PARAM0, val0);
	CSR_WRITE_2(sc, WI_PARAM1, val1);
	CSR_WRITE_2(sc, WI_PARAM2, val2);
	CSR_WRITE_2(sc, WI_COMMAND, cmd);

	if (cmd == WI_CMD_INI) {
		/* XXX: should sleep here. */
		DELAY(100*1000);
	}
	/* wait for the cmd completed bit */
	for (i = 0; i < WI_TIMEOUT; i++) {
		if (CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_CMD)
			break;
		DELAY(WI_DELAY);
	}

	status = CSR_READ_2(sc, WI_STATUS);

	/* Ack the command */
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_CMD);

	if (i == WI_TIMEOUT) {
		printf("%s: command timed out, cmd=0x%x, arg=0x%x\n",
		    sc->sc_dev.dv_xname, cmd, val0);
		return ETIMEDOUT;
	}

	if (status & WI_STAT_CMD_RESULT) {
		printf("%s: command failed, cmd=0x%x, arg=0x%x\n",
		    sc->sc_dev.dv_xname, cmd, val0);
		return EIO;
	}
	return 0;
}

static int
wi_seek_bap(struct wi_softc *sc, int id, int off)
{
	int i, status;

	CSR_WRITE_2(sc, WI_SEL0, id);
	CSR_WRITE_2(sc, WI_OFF0, off);

	for (i = 0; ; i++) {
		status = CSR_READ_2(sc, WI_OFF0);
		if ((status & WI_OFF_BUSY) == 0)
			break;
		if (i == WI_TIMEOUT) {
			printf("%s: timeout in wi_seek to %x/%x\n",
			    sc->sc_dev.dv_xname, id, off);
			sc->sc_bap_off = WI_OFF_ERR;	/* invalidate */
			return ETIMEDOUT;
		}
		DELAY(1);
	}
	if (status & WI_OFF_ERR) {
		printf("%s: failed in wi_seek to %x/%x\n",
		    sc->sc_dev.dv_xname, id, off);
		sc->sc_bap_off = WI_OFF_ERR;	/* invalidate */
		return EIO;
	}
	sc->sc_bap_id = id;
	sc->sc_bap_off = off;
	return 0;
}

static int
wi_read_bap(struct wi_softc *sc, int id, int off, void *buf, int buflen)
{
	int error, cnt;

	if (buflen == 0)
		return 0;
	if (id != sc->sc_bap_id || off != sc->sc_bap_off) {
		if ((error = wi_seek_bap(sc, id, off)) != 0)
			return error;
	}
	cnt = (buflen + 1) / 2;
	CSR_READ_MULTI_STREAM_2(sc, WI_DATA0, (u_int16_t *)buf, cnt);
	sc->sc_bap_off += cnt * 2;
	return 0;
}

static int
wi_write_bap(struct wi_softc *sc, int id, int off, void *buf, int buflen)
{
	int error, cnt;

	if (buflen == 0)
		return 0;

#ifdef WI_HERMES_AUTOINC_WAR
  again:
#endif
	if (id != sc->sc_bap_id || off != sc->sc_bap_off) {
		if ((error = wi_seek_bap(sc, id, off)) != 0)
			return error;
	}
	cnt = (buflen + 1) / 2;
	CSR_WRITE_MULTI_STREAM_2(sc, WI_DATA0, (u_int16_t *)buf, cnt);
	sc->sc_bap_off += cnt * 2;

#ifdef WI_HERMES_AUTOINC_WAR
	/*
	 * According to the comments in the HCF Light code, there is a bug
	 * in the Hermes (or possibly in certain Hermes firmware revisions)
	 * where the chip's internal autoincrement counter gets thrown off
	 * during data writes:  the autoincrement is missed, causing one
	 * data word to be overwritten and subsequent words to be written to
	 * the wrong memory locations. The end result is that we could end
	 * up transmitting bogus frames without realizing it. The workaround
	 * for this is to write a couple of extra guard words after the end
	 * of the transfer, then attempt to read then back. If we fail to
	 * locate the guard words where we expect them, we preform the
	 * transfer over again.
	 */
	if ((sc->sc_flags & WI_FLAGS_BUG_AUTOINC) && (id & 0xf000) == 0) {
		CSR_WRITE_2(sc, WI_DATA0, 0x1234);
		CSR_WRITE_2(sc, WI_DATA0, 0x5678);
		wi_seek_bap(sc, id, sc->sc_bap_off);
		sc->sc_bap_off = WI_OFF_ERR;	/* invalidate */
		if (CSR_READ_2(sc, WI_DATA0) != 0x1234 ||
		    CSR_READ_2(sc, WI_DATA0) != 0x5678) {
			printf("%s: detect auto increment bug, try again\n",
			    sc->sc_dev.dv_xname);
			goto again;
		}
	}
#endif
	return 0;
}

static int
wi_mwrite_bap(struct wi_softc *sc, int id, int off, struct mbuf *m0, int totlen)
{
	int error, len;
	struct mbuf *m;

	for (m = m0; m != NULL && totlen > 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;

		len = min(m->m_len, totlen);

		if (((u_long)m->m_data) % 2 != 0 || len % 2 != 0) {
			m_copydata(m, 0, totlen, (caddr_t)&sc->sc_txbuf);
			return wi_write_bap(sc, id, off, (caddr_t)&sc->sc_txbuf,
			    totlen);
		}

		if ((error = wi_write_bap(sc, id, off, m->m_data, len)) != 0)
			return error;

		off += m->m_len;
		totlen -= len;
	}
	return 0;
}

static int
wi_alloc_fid(struct wi_softc *sc, int len, int *idp)
{
	int i;

	if (wi_cmd(sc, WI_CMD_ALLOC_MEM, len, 0, 0)) {
		printf("%s: failed to allocate %d bytes on NIC\n",
		    sc->sc_dev.dv_xname, len);
		return ENOMEM;
	}

	for (i = 0; i < WI_TIMEOUT; i++) {
		if (CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_ALLOC)
			break;
		if (i == WI_TIMEOUT) {
			printf("%s: timeout in alloc\n", sc->sc_dev.dv_xname);
			return ETIMEDOUT;
		}
		DELAY(1);
	}
	*idp = CSR_READ_2(sc, WI_ALLOC_FID);
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_ALLOC);
	return 0;
}

static int
wi_read_rid(struct wi_softc *sc, int rid, void *buf, int *buflenp)
{
	int error, len;
	u_int16_t ltbuf[2];

	/* Tell the NIC to enter record read mode. */
	error = wi_cmd(sc, WI_CMD_ACCESS | WI_ACCESS_READ, rid, 0, 0);
	if (error)
		return error;

	error = wi_read_bap(sc, rid, 0, ltbuf, sizeof(ltbuf));
	if (error)
		return error;

	if (le16toh(ltbuf[1]) != rid) {
		printf("%s: record read mismatch, rid=%x, got=%x\n",
		    sc->sc_dev.dv_xname, rid, le16toh(ltbuf[1]));
		return EIO;
	}
	len = (le16toh(ltbuf[0]) - 1) * 2;	 /* already got rid */
	if (*buflenp < len) {
		printf("%s: record buffer is too small, "
		    "rid=%x, size=%d, len=%d\n",
		    sc->sc_dev.dv_xname, rid, *buflenp, len);
		return ENOSPC;
	}
	*buflenp = len;
	return wi_read_bap(sc, rid, sizeof(ltbuf), buf, len);
}

static int
wi_write_rid(struct wi_softc *sc, int rid, void *buf, int buflen)
{
	int error;
	u_int16_t ltbuf[2];

	ltbuf[0] = htole16((buflen + 1) / 2 + 1);	 /* includes rid */
	ltbuf[1] = htole16(rid);

	error = wi_write_bap(sc, rid, 0, ltbuf, sizeof(ltbuf));
	if (error)
		return error;
	error = wi_write_bap(sc, rid, sizeof(ltbuf), buf, buflen);
	if (error)
		return error;

	return wi_cmd(sc, WI_CMD_ACCESS | WI_ACCESS_WRITE, rid, 0, 0);
}

static void
wi_rssadapt_updatestats_cb(void *arg, struct ieee80211_node *ni)
{
	struct wi_node *wn = (void*)ni;
	ieee80211_rssadapt_updatestats(&wn->wn_rssadapt);
}

static void
wi_rssadapt_updatestats(void *arg)
{
	struct wi_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	ieee80211_iterate_nodes(ic, wi_rssadapt_updatestats_cb, arg);
	if (ic->ic_opmode != IEEE80211_M_MONITOR &&
	    ic->ic_state == IEEE80211_S_RUN)
		callout_reset(&sc->sc_rssadapt_ch, hz / 10,
		    wi_rssadapt_updatestats, arg);
}

static int
wi_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct wi_softc *sc = ic->ic_softc;
	struct ieee80211_node *ni = ic->ic_bss;
	int buflen;
	u_int16_t val;
	struct wi_ssid ssid;
	struct wi_macaddr bssid, old_bssid;
	enum ieee80211_state ostate;
#ifdef WI_DEBUG
	static const char *stname[] =
	    { "INIT", "SCAN", "AUTH", "ASSOC", "RUN" };
#endif /* WI_DEBUG */

	ostate = ic->ic_state;
	DPRINTF(("wi_newstate: %s -> %s\n", stname[ostate], stname[nstate]));

	switch (nstate) {
	case IEEE80211_S_INIT:
		if (ic->ic_opmode != IEEE80211_M_MONITOR)
			callout_stop(&sc->sc_rssadapt_ch);
		ic->ic_flags &= ~IEEE80211_F_SIBSS;
		sc->sc_flags &= ~WI_FLAGS_OUTRANGE;
		return (*sc->sc_newstate)(ic, nstate, arg);

	case IEEE80211_S_RUN:
		sc->sc_flags &= ~WI_FLAGS_OUTRANGE;
		buflen = IEEE80211_ADDR_LEN;
		IEEE80211_ADDR_COPY(old_bssid.wi_mac_addr, ni->ni_bssid);
		wi_read_rid(sc, WI_RID_CURRENT_BSSID, &bssid, &buflen);
		IEEE80211_ADDR_COPY(ni->ni_bssid, &bssid);
		IEEE80211_ADDR_COPY(ni->ni_macaddr, &bssid);
		buflen = sizeof(val);
		wi_read_rid(sc, WI_RID_CURRENT_CHAN, &val, &buflen);
		if (!isset(ic->ic_chan_avail, le16toh(val)))
			panic("%s: invalid channel %d\n", sc->sc_dev.dv_xname,
			    le16toh(val));
		ni->ni_chan = &ic->ic_channels[le16toh(val)];

		if (IEEE80211_ADDR_EQ(old_bssid.wi_mac_addr, ni->ni_bssid))
			sc->sc_false_syns++;
		else
			sc->sc_false_syns = 0;

		if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
			ni->ni_esslen = ic->ic_des_esslen;
			memcpy(ni->ni_essid, ic->ic_des_essid, ni->ni_esslen);
			ni->ni_rates = ic->ic_sup_rates[
			    ieee80211_chan2mode(ic, ni->ni_chan)];
			ni->ni_intval = ic->ic_lintval;
			ni->ni_capinfo = IEEE80211_CAPINFO_ESS;
			if (ic->ic_flags & IEEE80211_F_WEPON)
				ni->ni_capinfo |= IEEE80211_CAPINFO_PRIVACY;
		} else {
			buflen = sizeof(ssid);
			wi_read_rid(sc, WI_RID_CURRENT_SSID, &ssid, &buflen);
			ni->ni_esslen = le16toh(ssid.wi_len);
			if (ni->ni_esslen > IEEE80211_NWID_LEN)
				ni->ni_esslen = IEEE80211_NWID_LEN;	/*XXX*/
			memcpy(ni->ni_essid, ssid.wi_ssid, ni->ni_esslen);
			ni->ni_rates = ic->ic_sup_rates[
			    ieee80211_chan2mode(ic, ni->ni_chan)]; /*XXX*/
		}
		if (ic->ic_opmode != IEEE80211_M_MONITOR)
			callout_reset(&sc->sc_rssadapt_ch, hz / 10,
			    wi_rssadapt_updatestats, sc);
		break;

	case IEEE80211_S_SCAN:
	case IEEE80211_S_AUTH:
	case IEEE80211_S_ASSOC:
		break;
	}

	ic->ic_state = nstate;
	/* skip standard ieee80211 handling */
	return 0;
}

static int
wi_set_tim(struct ieee80211com *ic, int aid, int which)
{
	struct wi_softc *sc = ic->ic_softc;

	aid &= ~0xc000;
	if (which)
		aid |= 0x8000;

	return wi_write_val(sc, WI_RID_SET_TIM, aid);
}

static int
wi_scan_ap(struct wi_softc *sc, u_int16_t chanmask, u_int16_t txrate)
{
	int error = 0;
	u_int16_t val[2];

	if (!sc->sc_enabled)
		return ENXIO;
	switch (sc->sc_firmware_type) {
	case WI_LUCENT:
		(void)wi_cmd(sc, WI_CMD_INQUIRE, WI_INFO_SCAN_RESULTS, 0, 0);
		break;
	case WI_INTERSIL:
		val[0] = htole16(chanmask);	/* channel */
		val[1] = htole16(txrate);	/* tx rate */
		error = wi_write_rid(sc, WI_RID_SCAN_REQ, val, sizeof(val));
		break;
	case WI_SYMBOL:
		/*
		 * XXX only supported on 3.x ?
		 */
		val[0] = BSCAN_BCAST | BSCAN_ONETIME;
		error = wi_write_rid(sc, WI_RID_BCAST_SCAN_REQ,
		    val, sizeof(val[0]));
		break;
	}
	if (error == 0) {
		sc->sc_scan_timer = WI_SCAN_WAIT;
		sc->sc_ic.ic_if.if_timer = 1;
		DPRINTF(("wi_scan_ap: start scanning, "
			"chanmask 0x%x txrate 0x%x\n", chanmask, txrate));
	}
	return error;
}

static void
wi_scan_result(struct wi_softc *sc, int fid, int cnt)
{
#define	N(a)	(sizeof (a) / sizeof (a[0]))
	int i, naps, off, szbuf;
	struct wi_scan_header ws_hdr;	/* Prism2 header */
	struct wi_scan_data_p2 ws_dat;	/* Prism2 scantable*/
	struct wi_apinfo *ap;

	off = sizeof(u_int16_t) * 2;
	memset(&ws_hdr, 0, sizeof(ws_hdr));
	switch (sc->sc_firmware_type) {
	case WI_INTERSIL:
		wi_read_bap(sc, fid, off, &ws_hdr, sizeof(ws_hdr));
		off += sizeof(ws_hdr);
		szbuf = sizeof(struct wi_scan_data_p2);
		break;
	case WI_SYMBOL:
		szbuf = sizeof(struct wi_scan_data_p2) + 6;
		break;
	case WI_LUCENT:
		szbuf = sizeof(struct wi_scan_data);
		break;
	default:
		printf("%s: wi_scan_result: unknown firmware type %u\n",
		    sc->sc_dev.dv_xname, sc->sc_firmware_type);
		naps = 0;
		goto done;
	}
	naps = (cnt * 2 + 2 - off) / szbuf;
	if (naps > N(sc->sc_aps))
		naps = N(sc->sc_aps);
	sc->sc_naps = naps;
	/* Read Data */
	ap = sc->sc_aps;
	memset(&ws_dat, 0, sizeof(ws_dat));
	for (i = 0; i < naps; i++, ap++) {
		wi_read_bap(sc, fid, off, &ws_dat,
		    (sizeof(ws_dat) < szbuf ? sizeof(ws_dat) : szbuf));
		DPRINTF2(("wi_scan_result: #%d: off %d bssid %s\n", i, off,
		    ether_sprintf(ws_dat.wi_bssid)));
		off += szbuf;
		ap->scanreason = le16toh(ws_hdr.wi_reason);
		memcpy(ap->bssid, ws_dat.wi_bssid, sizeof(ap->bssid));
		ap->channel = le16toh(ws_dat.wi_chid);
		ap->signal  = le16toh(ws_dat.wi_signal);
		ap->noise   = le16toh(ws_dat.wi_noise);
		ap->quality = ap->signal - ap->noise;
		ap->capinfo = le16toh(ws_dat.wi_capinfo);
		ap->interval = le16toh(ws_dat.wi_interval);
		ap->rate    = le16toh(ws_dat.wi_rate);
		ap->namelen = le16toh(ws_dat.wi_namelen);
		if (ap->namelen > sizeof(ap->name))
			ap->namelen = sizeof(ap->name);
		memcpy(ap->name, ws_dat.wi_name, ap->namelen);
	}
done:
	/* Done scanning */
	sc->sc_scan_timer = 0;
	DPRINTF(("wi_scan_result: scan complete: ap %d\n", naps));
#undef N
}

static void
wi_dump_pkt(struct wi_frame *wh, struct ieee80211_node *ni, int rssi)
{
	ieee80211_dump_pkt((u_int8_t *) &wh->wi_whdr, sizeof(wh->wi_whdr),
	    ni	? ni->ni_rates.rs_rates[ni->ni_txrate] & IEEE80211_RATE_VAL
		: -1,
	    rssi);
	printf(" status 0x%x rx_tstamp1 %u rx_tstamp0 0x%u rx_silence %u\n",
		le16toh(wh->wi_status), le16toh(wh->wi_rx_tstamp1),
		le16toh(wh->wi_rx_tstamp0), wh->wi_rx_silence);
	printf(" rx_signal %u rx_rate %u rx_flow %u\n",
		wh->wi_rx_signal, wh->wi_rx_rate, wh->wi_rx_flow);
	printf(" tx_rtry %u tx_rate %u tx_ctl 0x%x dat_len %u\n",
		wh->wi_tx_rtry, wh->wi_tx_rate,
		le16toh(wh->wi_tx_ctl), le16toh(wh->wi_dat_len));
	printf(" ehdr dst %s src %s type 0x%x\n",
		ether_sprintf(wh->wi_ehdr.ether_dhost),
		ether_sprintf(wh->wi_ehdr.ether_shost),
		wh->wi_ehdr.ether_type);
}
