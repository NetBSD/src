/*	$NetBSD: wi.c,v 1.84 2002/09/23 14:31:27 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wi.c,v 1.84 2002/09/23 14:31:27 thorpej Exp $");

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
#include <net/if_ieee80211.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

static void wi_tap_802_11_plus	__P((struct wi_softc *, struct mbuf *));
static void wi_tap_802_3	__P((struct wi_softc *, struct mbuf *,
				     struct wi_frame *));
static void wi_reset		__P((struct wi_softc *));
static int wi_ioctl		__P((struct ifnet *, u_long, caddr_t));
static void wi_start		__P((struct ifnet *));
static void wi_watchdog		__P((struct ifnet *));
static int wi_init		__P((struct ifnet *));
static void wi_stop		__P((struct ifnet *, int));
static void wi_rxeof		__P((struct wi_softc *));
static void wi_txeof		__P((struct wi_softc *, int));
static void wi_update_stats	__P((struct wi_softc *));
static void wi_setmulti		__P((struct wi_softc *));

static int wi_cmd		__P((struct wi_softc *, int, int, int, int));
static int wi_read_record	__P((struct wi_softc *, struct wi_ltv_gen *));
static int wi_write_record	__P((struct wi_softc *, struct wi_ltv_gen *));
static int wi_read_data		__P((struct wi_softc *, int,
					int, caddr_t, int));
#if defined(OPTIMIZE_RW_DATA)
static void wi_rewind		__P((struct wi_softc *));
#endif
static int wi_write_data	__P((struct wi_softc *, int,
					int, caddr_t, int));
static int wi_seek		__P((struct wi_softc *, int, int, int));
static int wi_alloc_nicmem	__P((struct wi_softc *, int, int *));
static void wi_inquire		__P((void *));
static void wi_wait_scan	__P((void *));
static int wi_setdef		__P((struct wi_softc *, struct wi_req *));
static int wi_getdef		__P((struct wi_softc *, struct wi_req *));

static int wi_media_change __P((struct ifnet *));
static void wi_media_status __P((struct ifnet *, struct ifmediareq *));

static void wi_get_id		__P((struct wi_softc *));

static int wi_set_ssid __P((struct ieee80211_nwid *, u_int8_t *, int));
static void wi_request_fill_ssid __P((struct wi_req *,
    struct ieee80211_nwid *));
static int wi_write_ssid __P((struct wi_softc *, int, struct wi_req *,
    struct ieee80211_nwid *));
static int wi_set_nwkey __P((struct wi_softc *, struct ieee80211_nwkey *));
static int wi_get_nwkey __P((struct wi_softc *, struct ieee80211_nwkey *));
static int wi_sync_media __P((struct wi_softc *, int, int));
static int wi_set_pm __P((struct wi_softc *, struct ieee80211_power *));
static int wi_get_pm __P((struct wi_softc *, struct ieee80211_power *));
static int wi_get_channel __P((struct wi_softc *, struct ieee80211_channel *));
static int wi_set_channel __P((struct wi_softc *, struct ieee80211_channel *));
static int wi_join_bss __P((struct wi_softc *));

struct wi_card_ident wi_card_ident[] = {
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

static const u_int8_t zero_bssid[6] = {0, 0, 0, 0, 0, 0};

int
wi_attach(sc)
	struct wi_softc *sc;
{
	struct ifnet *ifp = sc->sc_ifp;
	const char *sep = "";
	int i, nrate;
	u_int8_t *r;
	struct wi_ltv_macaddr   mac;
	struct wi_ltv_gen       gen;
	struct wi_ltv_str	rate;
	static const u_int8_t empty_macaddr[ETHER_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int s;

	s = splnet();

	callout_init(&sc->wi_inquire_ch);
	callout_init(&sc->wi_scan_sh);

	/* Make sure interrupts are disabled. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);

	/* Reset the NIC. */
	wi_reset(sc);

	memset(&mac, 0, sizeof(mac));
	/* Read the station address. */
	mac.wi_type = WI_RID_MAC_NODE;
	mac.wi_len = 4;
	wi_read_record(sc, (struct wi_ltv_gen *)&mac);
	memcpy(sc->sc_macaddr, mac.wi_mac_addr, ETHER_ADDR_LEN);

	/*
	 * Check if we got anything meaningful.
	 *
	 * Is it really enough just checking against null ethernet address?
	 * Or, check against possible vendor?  XXX.
	 */
	if (memcmp(sc->sc_macaddr, empty_macaddr, ETHER_ADDR_LEN) == 0) {
		printf("could not get mac address, attach failed\n");
		splx(s);
		return 1;
	}

	printf(" 802.11 address %s\n", ether_sprintf(sc->sc_macaddr));

	/* Read NIC identification */
	wi_get_id(sc);

	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = wi_start;
	ifp->if_ioctl = wi_ioctl;
	ifp->if_watchdog = wi_watchdog;
	ifp->if_init = wi_init;
	ifp->if_stop = wi_stop;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef IFF_NOTRAILERS
	ifp->if_flags |= IFF_NOTRAILERS;
#endif
	IFQ_SET_READY(&ifp->if_snd);

	(void)wi_set_ssid(&sc->wi_nodeid, WI_DEFAULT_NODENAME,
	    sizeof(WI_DEFAULT_NODENAME) - 1);
	(void)wi_set_ssid(&sc->wi_netid, WI_DEFAULT_NETNAME,
	    sizeof(WI_DEFAULT_NETNAME) - 1);
	(void)wi_set_ssid(&sc->wi_ibssid, WI_DEFAULT_IBSS,
	    sizeof(WI_DEFAULT_IBSS) - 1);

	sc->wi_portnum = WI_DEFAULT_PORT;
	sc->wi_ptype = WI_PORTTYPE_BSS;
	sc->wi_ap_density = WI_DEFAULT_AP_DENSITY;
	sc->wi_rts_thresh = WI_DEFAULT_RTS_THRESH;
	sc->wi_tx_rate = WI_DEFAULT_TX_RATE;
	sc->wi_max_data_len = WI_DEFAULT_DATALEN;
	sc->wi_create_ibss = WI_DEFAULT_CREATE_IBSS;
	sc->wi_pm_enabled = WI_DEFAULT_PM_ENABLED;
	sc->wi_max_sleep = WI_DEFAULT_MAX_SLEEP;
	sc->wi_roaming = WI_DEFAULT_ROAMING;
	sc->wi_authtype = WI_DEFAULT_AUTHTYPE;

	/*
	 * Read the default channel from the NIC. This may vary
	 * depending on the country where the NIC was purchased, so
	 * we can't hard-code a default and expect it to work for
	 * everyone.
	 */
	gen.wi_type = WI_RID_OWN_CHNL;
	gen.wi_len = 2;
	wi_read_record(sc, &gen);
	sc->wi_create_channel = sc->wi_join_channel = le16toh(gen.wi_val);

	memset((char *)&sc->wi_stats, 0, sizeof(sc->wi_stats));

	/* AP info was filled with 0 */
	memset((char *)&sc->wi_aps, 0, sizeof(sc->wi_aps));
	sc->wi_scanning = 0;
	sc->wi_naps = 0;

	/*
	 * Set flags based on firmware version.
	 */
	switch (sc->sc_firmware_type) {
	case WI_LUCENT:
		sc->wi_flags |= WI_FLAGS_HAS_ROAMING;
		if (sc->sc_sta_firmware_ver >= 60000)
			sc->wi_flags |= WI_FLAGS_HAS_MOR;
		if (sc->sc_sta_firmware_ver >= 60006) {
			sc->wi_flags |= WI_FLAGS_HAS_IBSS; 
			sc->wi_flags |= WI_FLAGS_HAS_CREATE_IBSS;
		}
		sc->wi_ibss_port = htole16(1);
		break;

	case WI_INTERSIL:
		sc->wi_flags |= WI_FLAGS_HAS_ROAMING;
		if (sc->sc_sta_firmware_ver >= 800) {
			sc->wi_flags |= WI_FLAGS_HAS_HOSTAP;
			sc->wi_flags |= WI_FLAGS_HAS_IBSS;
#if 0 /* Prism firmware interprets Create IBSS differently than we thought. */
			sc->wi_flags |= WI_FLAGS_HAS_CREATE_IBSS;
#endif
		}
		sc->wi_ibss_port = htole16(0);
		break;

	case WI_SYMBOL:
		sc->wi_flags |= WI_FLAGS_HAS_DIVERSITY;
		if (sc->sc_sta_firmware_ver >= 20000)
			sc->wi_flags |= WI_FLAGS_HAS_IBSS;
#if 0 /* Prism firmware interprets Create IBSS differently than we thought. */
		if (sc->sc_sta_firmware_ver >= 25000)
			sc->wi_flags |= WI_FLAGS_HAS_CREATE_IBSS;
#endif
		sc->wi_ibss_port = htole16(4);
		break;
	}

	/*
	 * Find out if we support WEP on this card.
	 */
	gen.wi_type = WI_RID_WEP_AVAIL;
	gen.wi_len = 2;
	if (wi_read_record(sc, &gen) == 0 &&
	    gen.wi_val != le16toh(0))
		sc->wi_flags |= WI_FLAGS_HAS_WEP;

	gen.wi_type = WI_RID_CHANNEL_LIST;
	gen.wi_len = 2;
	if (wi_read_record(sc, &gen) == 0)
		sc->wi_channels = le16toh(gen.wi_val);
	else
		sc->wi_channels = (1 << 14) - 1; /* assume all 14 channels */

	/* Find supported rates. */
	rate.wi_type = WI_RID_DATA_RATES;
	rate.wi_len = 6;
	if (wi_read_record(sc, (struct wi_ltv_gen *)&rate) == 0) {
		nrate = le16toh(rate.wi_str[0]);
		r = (u_int8_t *)&rate.wi_str[1];
		for (i = 0; i < nrate; i++) {
			switch (r[i] & IEEE80211_RATE_VAL) {
			case 2:
				sc->wi_supprates |= WI_SUPPRATES_1M;
				break;
			case 4:
				sc->wi_supprates |= WI_SUPPRATES_2M;
				break;
			case 11:
				sc->wi_supprates |= WI_SUPPRATES_5M;
				break;
			case 22:
				sc->wi_supprates |= WI_SUPPRATES_11M;
				break;
			}
		}
	}

	ifmedia_init(&sc->sc_media, 0, wi_media_change, wi_media_status);
	if (sc->wi_supprates != 0)
		printf("%s: supported rates: ", sc->sc_dev.dv_xname);
#define	ADD(s, o)	ifmedia_add(&sc->sc_media, \
	IFM_MAKEWORD(IFM_IEEE80211, (s), (o), 0), 0, NULL)
#define	PRINT(n)	printf("%s%s", sep, (n)); sep = ", "
	ADD(IFM_AUTO, 0);
	if (sc->wi_flags & WI_FLAGS_HAS_HOSTAP)
		ADD(IFM_AUTO, IFM_IEEE80211_HOSTAP);
	if (sc->wi_flags & WI_FLAGS_HAS_IBSS)
		ADD(IFM_AUTO, IFM_IEEE80211_ADHOC);
	ADD(IFM_AUTO, IFM_IEEE80211_ADHOC | IFM_FLAG0);
	if (sc->wi_supprates & WI_SUPPRATES_1M) {
		PRINT("1Mbps");
		ADD(IFM_IEEE80211_DS1, 0);
		if (sc->wi_flags & WI_FLAGS_HAS_HOSTAP)
			ADD(IFM_IEEE80211_DS1, IFM_IEEE80211_HOSTAP);
		if (sc->wi_flags & WI_FLAGS_HAS_IBSS)
			ADD(IFM_IEEE80211_DS1, IFM_IEEE80211_ADHOC);
		ADD(IFM_IEEE80211_DS1, IFM_IEEE80211_ADHOC | IFM_FLAG0);
	}
	if (sc->wi_supprates & WI_SUPPRATES_2M) {
		PRINT("2Mbps");
		ADD(IFM_IEEE80211_DS2, 0);
		if (sc->wi_flags & WI_FLAGS_HAS_HOSTAP)
			ADD(IFM_IEEE80211_DS2, IFM_IEEE80211_HOSTAP);
		if (sc->wi_flags & WI_FLAGS_HAS_IBSS)
			ADD(IFM_IEEE80211_DS2, IFM_IEEE80211_ADHOC);
		ADD(IFM_IEEE80211_DS2, IFM_IEEE80211_ADHOC | IFM_FLAG0);
	}
	if (sc->wi_supprates & WI_SUPPRATES_5M) {
		PRINT("5.5Mbps");
		ADD(IFM_IEEE80211_DS5, 0);
		if (sc->wi_flags & WI_FLAGS_HAS_HOSTAP)
			ADD(IFM_IEEE80211_DS5, IFM_IEEE80211_HOSTAP);
		if (sc->wi_flags & WI_FLAGS_HAS_IBSS)
			ADD(IFM_IEEE80211_DS5, IFM_IEEE80211_ADHOC);
		ADD(IFM_IEEE80211_DS5, IFM_IEEE80211_ADHOC | IFM_FLAG0);
	}
	if (sc->wi_supprates & WI_SUPPRATES_11M) {
		PRINT("11Mbps");
		ADD(IFM_IEEE80211_DS11, 0);
		if (sc->wi_flags & WI_FLAGS_HAS_HOSTAP)
			ADD(IFM_IEEE80211_DS11, IFM_IEEE80211_HOSTAP);
		if (sc->wi_flags & WI_FLAGS_HAS_IBSS)
			ADD(IFM_IEEE80211_DS11, IFM_IEEE80211_ADHOC);
		ADD(IFM_IEEE80211_DS11, IFM_IEEE80211_ADHOC | IFM_FLAG0);
	}
	if (sc->wi_supprates != 0)
		printf("\n");
	ifmedia_set(&sc->sc_media, IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO, 0, 0));
#undef ADD
#undef PRINT

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_macaddr);

	ifp->if_baudrate = IF_Mbps(2);

#if NBPFILTER > 0
	bpfattach2(ifp, DLT_IEEE802_11, sizeof(struct ieee80211_frame),
	           (caddr_t*) &sc->sc_bpf80211);
	if (sc->sc_firmware_type == WI_INTERSIL ||
	    sc->sc_firmware_type == WI_SYMBOL) {
		bpfattach2(ifp, DLT_PRISM_HEADER,
		           WI_HWSPEC_END + sizeof(struct ieee80211_frame),
			   (caddr_t*) &sc->sc_bpf80211plus);
	}
#endif

	/* Attach is successful. */
	sc->sc_attached = 1;

	splx(s);
	return 0;
}

static void
wi_tap_802_11_plus(struct wi_softc *sc, struct mbuf *m)
{
	if (sc->sc_bpf80211plus) {
		bpf_mtap((caddr_t) sc->sc_bpf80211plus, m);
	}
	if (sc->sc_bpf80211) {

		/* trim off hardware-specific frame header */
		m->m_pkthdr.len -= WI_HWSPEC_END;
		m->m_len -= WI_HWSPEC_END;
		m->m_data += WI_HWSPEC_END;

		bpf_mtap((caddr_t) sc->sc_bpf80211, m);

		/* restore hardware-specific frame header */
		m->m_data -= WI_HWSPEC_END;
		m->m_len += WI_HWSPEC_END;
		m->m_pkthdr.len += WI_HWSPEC_END;
	}
}

static void
wi_tap_802_3(struct wi_softc *sc, struct mbuf *m, struct wi_frame *hwframe)
{
	int encrypted;
	struct llc llc;
	struct mbuf m0, m1;
	struct ether_header *eh;

	/* hand up 802.3 frame */
	if (sc->sc_ifp->if_bpf) {
		bpf_mtap(sc->sc_ifp->if_bpf, m);
	}

	if (m->m_len < sizeof(struct ether_header)) {
		printf("%s: inconsistent 802.3 mbuf, only %d bytes",
		    sc->sc_dev.dv_xname, m->m_len);
		return;
	}

	eh = mtod(m, struct ether_header *);

	llc.llc_dsap = llc.llc_ssap = LLC_SNAP_LSAP;
	llc.llc_control = LLC_UI;
	llc.llc_snap.org_code[0] = 0;
	llc.llc_snap.org_code[1] = 0;
	llc.llc_snap.org_code[2] = 0;
	llc.llc_snap.ether_type = eh->ether_type;

	/* add to chain one mbuf for hardware-specific and 802.11 header,
	 * one for SNAP.
	 */
	m0.m_pkthdr = m->m_pkthdr;
	m0.m_flags = m->m_flags & M_COPYFLAGS;
	m0.m_next = &m1;

	m1.m_next = m;
	m1.m_data = (caddr_t) &llc;
	m1.m_len = sizeof(struct llc);

	/* omit 802.3 header */
	m->m_len -= sizeof(struct ether_header);
	m->m_data += sizeof(struct ether_header);

	/* do not indicate WEP-encryption to bpf. */
	encrypted = hwframe->wi_frame_ctl & htole16(WI_FCTL_WEP);
	hwframe->wi_frame_ctl &= ~htole16(WI_FCTL_WEP);

	/* hand up hardware-specific frame */
	if (sc->sc_bpf80211plus) {

		m0.m_data = (caddr_t) hwframe;
		m0.m_len = WI_SHORT_802_11_END;

		m0.m_pkthdr.len = m->m_pkthdr.len + WI_SHORT_802_11_END +
		    sizeof(struct llc) - sizeof(struct ether_header);

		if ((hwframe->wi_frame_ctl & htole16(WI_FCTL_FROMDS)) &&
		    (hwframe->wi_frame_ctl & htole16(WI_FCTL_TODS))) {
			m0.m_pkthdr.len +=
			    WI_LONG_802_11_END - WI_SHORT_802_11_END;
			m0.m_len += WI_LONG_802_11_END - WI_SHORT_802_11_END;
		}
		bpf_mtap((caddr_t) sc->sc_bpf80211plus, &m0);
	}

	/* hand up 802.11 frame */
	if (sc->sc_bpf80211) {

		m0.m_data = (caddr_t) hwframe + WI_802_11_BEGIN;
		m0.m_len = (WI_SHORT_802_11_END - WI_802_11_BEGIN);

		m0.m_pkthdr.len = m->m_pkthdr.len +
		    (WI_SHORT_802_11_END - WI_802_11_BEGIN) +
		    sizeof(struct llc) - sizeof(struct ether_header);

		if ((hwframe->wi_frame_ctl & htole16(WI_FCTL_FROMDS)) &&
		    (hwframe->wi_frame_ctl & htole16(WI_FCTL_TODS))) {
			m0.m_pkthdr.len +=
			    WI_LONG_802_11_END - WI_SHORT_802_11_END;
			m0.m_len += WI_LONG_802_11_END - WI_SHORT_802_11_END;
		}
		bpf_mtap((caddr_t) sc->sc_bpf80211, &m0);
	}

	if (encrypted) {
		/* restore WEP indication. */
		hwframe->wi_frame_ctl |= htole16(WI_FCTL_WEP);
	}

	/* restore 802.3 header */
	m->m_data -= sizeof(struct ether_header);
	m->m_len += sizeof(struct ether_header);

	return;
}

static int
wi_rx_rfc1042(struct wi_softc *sc, int id, struct wi_frame *frame,
              struct llc *llc, struct mbuf *m, int maxlen)
{
	struct ether_header	*eh;
	struct ifnet		*ifp;
	int		read_ofs, read_len;
	caddr_t		read_ptr;

	ifp = sc->sc_ifp;
	eh = mtod(m, struct ether_header *);

	read_len = le16toh(frame->wi_dat_len) - sizeof(struct llc);

	if (read_len + sizeof(struct ether_header) > maxlen) {
		printf("%s: wi_rx_rfc1042: oversized packet received "
		    "(wi_dat_len=%d, wi_status=0x%x)\n", sc->sc_dev.dv_xname,
		    le16toh(frame->wi_dat_len), le16toh(frame->wi_status));
		m_freem(m);
		ifp->if_ierrors++;
		return -1;
	}
	m->m_pkthdr.len = m->m_len = read_len + sizeof(struct ether_header);

	/* XXX use 802.11 dst, src, etc.? */
	(void)memcpy(&eh->ether_dhost, &frame->wi_dst_addr, ETHER_ADDR_LEN);
	(void)memcpy(&eh->ether_shost, &frame->wi_src_addr, ETHER_ADDR_LEN);
	eh->ether_type = llc->llc_snap.ether_type;

	read_ptr = (caddr_t) (eh + 1);
	read_ofs = sizeof(struct wi_frame) + sizeof(struct llc);

	if (wi_read_data(sc, id, read_ofs, read_ptr, read_len)) {
		m_freem(m);
		ifp->if_ierrors++;
		return -1;
	}

	return 0;
}

/* for wi_rx_ethii, llc points to 8 arbitrary bytes. */
static int
wi_rx_ethii(struct wi_softc *sc, int id, struct wi_frame *frame,
            struct llc *llc, struct mbuf *m, int maxlen)
{
	struct ifnet *ifp;
#if 0
	int		read_ofs, read_len;
	caddr_t		read_ptr;
#endif
	struct ether_header *eh;

	ifp = sc->sc_ifp;
	eh = mtod(m, struct ether_header *);

	if (le16toh(frame->wi_dat_len) + sizeof(struct ether_header) > maxlen) {
		printf("%s: wi_rx_ethii: oversized packet received "
		    "(wi_dat_len=%d, wi_status=0x%x)\n", sc->sc_dev.dv_xname,
		    le16toh(frame->wi_dat_len), le16toh(frame->wi_status));
		m_freem(m);
		ifp->if_ierrors++;
		return -1;
	}
	m->m_pkthdr.len = m->m_len = sizeof(struct ether_header) +
	    le16toh(frame->wi_dat_len);

#if 0
	(void)memcpy(&eh->ether_dhost, &frame->wi_dst_addr, ETHER_ADDR_LEN);
	(void)memcpy(&eh->ether_shost, &frame->wi_src_addr, ETHER_ADDR_LEN);
	eh->ether_type = frame->wi_len;

	read_ptr = mtod(m, caddr_t) + sizeof(struct ether_header) +
	    sizeof(struct llc);
	read_ofs = sizeof(struct wi_frame) + sizeof(struct llc);
	read_len = le16toh(frame->wi_dat_len) - sizeof(struct llc);

	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: frame->wi_dat_len = %d, frame->wi_len = %d\n",
		    sc->sc_dev.dv_xname, le16toh(frame->wi_dat_len),
		    frame->wi_len);

	(void)memcpy(mtod(m, caddr_t), (u_int8_t*)frame + WI_802_3_BEGIN,
	    sizeof(struct ether_header));

	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: eh->ether_type = %d\n", sc->sc_dev.dv_xname,
		    eh->ether_type);

	(void)memcpy(mtod(m, caddr_t) + sizeof(struct ether_header), llc,
	    sizeof(struct llc));

	if (wi_read_data(sc, id, read_ofs, read_ptr, read_len)) {
#else
	if (wi_read_data(sc, id, WI_802_3_BEGIN, mtod(m, caddr_t), m->m_len)) {
#endif
		m_freem(m);
		ifp->if_ierrors++;
		return -1;
	}
	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: 2nd eh->ether_type = %d\n", sc->sc_dev.dv_xname,
		    eh->ether_type);

	return 0;
}

static int
wi_rx_mgmt(struct wi_softc *sc, int id, struct wi_frame *frame, struct llc *llc,
           struct mbuf *m, int maxlen)
{
	int		read_ofs, read_len;
	caddr_t		read_ptr;
	struct ifnet		*ifp;

	ifp = sc->sc_ifp;

	if (le16toh(frame->wi_dat_len) + WI_SHORT_802_11_END > maxlen) {
		printf("%s: oversized packet received in "
		    "Host-AP mode (wi_dat_len=%d, wi_status=0x%x)\n",
		    sc->sc_dev.dv_xname,
		    le16toh(frame->wi_dat_len),
		    le16toh(frame->wi_status));
		m_freem(m);
		ifp->if_ierrors++;
		return -1;
	}

	/* Put the whole header in there. */
	(void)memcpy(mtod(m, void *), frame, sizeof(struct wi_frame));
	(void)memcpy(mtod(m, caddr_t) + WI_SHORT_802_11_END, llc,
	    sizeof(struct llc));

	read_ptr = mtod(m, caddr_t) + WI_SHORT_802_11_END + sizeof(struct llc);
	read_ofs = sizeof(struct wi_frame) + sizeof(struct llc);
	read_len = le16toh(frame->wi_dat_len) - sizeof(struct llc);

	if (wi_read_data(sc, id, read_ofs, read_ptr, read_len)) {
		m_freem(m);
		if (sc->sc_ethercom.ec_if.if_flags & IFF_DEBUG)
			printf("%s: Host-AP: failed to copy header\n",
			    sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
		return -1;
	}

	m->m_pkthdr.len = m->m_len =
	    WI_SHORT_802_11_END + le16toh(frame->wi_dat_len);

#if NBPFILTER > 0
	wi_tap_802_11_plus(sc, m);
#endif

	wihap_mgmt_input(sc, frame, m);

	return 0;
}

#if defined(OPTIMIZE_RW_DATA)
static void wi_rewind(sc)
	struct wi_softc		*sc;
{
	sc->wi_last_chan = -1;
	sc->wi_last_id = -1;
	sc->wi_last_offset = -1;
}
#endif

static void wi_rxeof(sc)
	struct wi_softc		*sc;
{
	struct ifnet		*ifp;
	struct mbuf		*m;
	int			id;
	caddr_t			olddata;
	int			maxlen;
	struct wi_frame		rx_frame;
	struct llc		llc;

	ifp = sc->sc_ifp;

#if defined(OPTIMIZE_RW_DATA)
	wi_rewind(sc);
#endif
	id = CSR_READ_2(sc, WI_RX_FID);

	/* First read in the frame header */
	if (wi_read_data(sc, id, 0, (caddr_t)&rx_frame,
	    sizeof(struct wi_frame))) {
		ifp->if_ierrors++;
		return;
	}
	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: rx_frame.wi_dat_len = %d\n", sc->sc_dev.dv_xname,
		    le16toh(rx_frame.wi_dat_len));
	/* Read optional LLC. */
	if (wi_read_data(sc, id, sizeof(struct wi_frame), (caddr_t)&llc,
	    min(sizeof(struct llc), le16toh(rx_frame.wi_dat_len)))) {
		ifp->if_ierrors++;
		return;
	}

	/* Drop undecryptable or packets with receive errors. */
	if (le16toh(rx_frame.wi_status) & WI_STAT_ERRSTAT) {
		ifp->if_ierrors++;
		return;
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		ifp->if_ierrors++;
		return;
	}
	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		ifp->if_ierrors++;
		return;
	}

	olddata = m->m_data;
	/* Align the data after the ethernet header */
	m->m_data = (caddr_t) ALIGN(m->m_data + sizeof(struct ether_header))
	    - sizeof(struct ether_header);
	maxlen = MCLBYTES - (m->m_data - olddata);
	m->m_pkthdr.rcvif = ifp;

	switch (le16toh(rx_frame.wi_status) & WI_RXSTAT_MSG_TYPE) {
	case WI_STAT_TUNNEL:
	case WI_STAT_1042:

		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: rx RFC1042\n", sc->sc_dev.dv_xname);

		/* Convert from RFC1042 encapsulation to Ethernet II
		 * encapsulation.
		 */
		if (wi_rx_rfc1042(sc, id, &rx_frame, &llc, m, maxlen))
			return;
		break;
	case WI_STAT_MGMT:
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: rx Mgmt\n", sc->sc_dev.dv_xname);

		if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
			(void)wi_rx_mgmt(sc, id, &rx_frame, &llc, m, maxlen);
			return;
		}
		/* fall through */
	case WI_STAT_NORMAL:

		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: rx Normal\n", sc->sc_dev.dv_xname);

		/* linux-wlan-ng reports that some RFC1042 frames
		 * are misidentified as Ethernet II frames. Check.
		 */
		if (le16toh(rx_frame.wi_dat_len) >= sizeof(struct llc) &&
		    llc.llc_dsap == LLC_SNAP_LSAP &&
		    llc.llc_ssap == LLC_SNAP_LSAP &&
		    llc.llc_control == LLC_UI) {
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: actually rx RFC1042\n",
				    sc->sc_dev.dv_xname);

			if (wi_rx_rfc1042(sc, id, &rx_frame, &llc, m, maxlen))
				return;
		} else if (wi_rx_ethii(sc, id, &rx_frame, &llc, m, maxlen))
				return;
		break;
	case WI_STAT_WMP_MSG:
		/* XXX hand these up with DLT_PRISM_HEADER? */
		printf("%s: dropping WMP frame\n", sc->sc_dev.dv_xname);
		break;
	default:
		/* XXX hand these up with DLT_PRISM_HEADER? */
		printf("%s: dropping unknown-encoded frame\n",
		       sc->sc_dev.dv_xname);
		break;
	}

	ifp->if_ipackets++;

#if NBPFILTER > 0
	/* Handle BPF listeners. */
	wi_tap_802_3(sc, m, &rx_frame);
#endif

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
		/* 
		 * Give Host-AP first crack at data packets.  If it
		 * decides to handle it (or drop it), it will return
		 * non-zero.  Otherwise, it is destined for this host.
		 */
		if (wihap_data_input(sc, &rx_frame, m))
			return;
	}

	/* Receive packet. */
	(*ifp->if_input)(ifp, m);
}

static void wi_txeof(sc, status)
	struct wi_softc	*sc;
	int		status;
{
	struct ifnet	*ifp = sc->sc_ifp;

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status & WI_EV_TX_EXC)
		ifp->if_oerrors++;
	else
		ifp->if_opackets++;

	return;
}

void wi_inquire(xsc)
	void			*xsc;
{
	struct wi_softc		*sc;
	struct ifnet		*ifp;
	int			s;

	sc = xsc;
	ifp = &sc->sc_ethercom.ec_if;

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return;

	KASSERT(sc->sc_enabled);

	callout_reset(&sc->wi_inquire_ch, hz * 60, wi_inquire, sc);

	/* Don't do this while we're transmitting */
	if (ifp->if_flags & IFF_OACTIVE)
		return;

	s = splnet();
	wi_cmd(sc, WI_CMD_INQUIRE, WI_INFO_COUNTERS, 0, 0);
	splx(s);
}

void wi_wait_scan(xsc)
	void			*xsc;
{
	struct wi_softc         *sc;  
	struct ifnet            *ifp;
	int			s, result;

	sc = xsc;
	ifp = &sc->sc_ethercom.ec_if;

	/* If not scanning, ignore */
	if (!sc->wi_scanning)
		return;

	s = splnet();

	/* Wait for sending complete to make INQUIRE */
	if (ifp->if_flags & IFF_OACTIVE) {
		callout_reset(&sc->wi_scan_sh, hz * 1, wi_wait_scan, sc);
		splx(s);
		return;
	}

	/* try INQUIRE */
	result = wi_cmd(sc, WI_CMD_INQUIRE, WI_INFO_SCAN_RESULTS, 0, 0);
	if (result == ETIMEDOUT)
		callout_reset(&sc->wi_scan_sh, hz * 1, wi_wait_scan, sc);

	splx(s);
}

void wi_update_stats(sc)
	struct wi_softc		*sc;
{
	struct wi_req		wreq;
	struct wi_ltv_gen	gen;
	struct wi_scan_header	ap2_header;	/* Prism2 header */
	struct wi_scan_data_p2	ap2;		/* Prism2 scantable*/
	struct wi_scan_data	ap;		/* Lucent scantable */
	struct wi_assoc		assoc;		/* Association Status */
	u_int16_t		id;
	struct ifnet		*ifp;
	u_int32_t		*ptr;
	int			len, naps, i, j;
	u_int16_t		t;

	ifp = &sc->sc_ethercom.ec_if;

#if defined(OPTIMIZE_RW_DATA)
	wi_rewind(sc);
#endif

	id = CSR_READ_2(sc, WI_INFO_FID);

	if (wi_seek(sc, id, 0, WI_BAP1)) {
		return;
	}
  
	gen.wi_len = CSR_READ_2(sc, WI_DATA1);
	gen.wi_type = CSR_READ_2(sc, WI_DATA1);

	switch (gen.wi_type) {
	case WI_INFO_SCAN_RESULTS:
	case WI_INFO_HOST_SCAN_RESULTS:
		if (gen.wi_len <= 3) {
			sc->wi_naps = 0;
			sc->wi_scanning = 0;
			break;
		}
		switch (sc->sc_firmware_type) {
		case WI_INTERSIL:
		case WI_SYMBOL:
			if (sc->sc_firmware_type == WI_INTERSIL) {
				naps = 2 * (gen.wi_len - 3) / sizeof(ap2);
				/* Read Header */
				for(j=0; j < sizeof(ap2_header) / 2; j++)
					((u_int16_t *)&ap2_header)[j] =
					    CSR_READ_2(sc, WI_DATA1);
			} else {	/* WI_SYMBOL */
				naps = 2 * (gen.wi_len - 1) / (sizeof(ap2) + 6);
				ap2_header.wi_reason = 0;
			}
			naps = naps > MAXAPINFO ? MAXAPINFO : naps;
			sc->wi_naps = naps;
			/* Read Data */
			for (i=0; i < naps; i++) {
				for(j=0; j < sizeof(ap2) / 2; j++)
					((u_int16_t *)&ap2)[j] =
						CSR_READ_2(sc, WI_DATA1);
				if (sc->sc_firmware_type == WI_SYMBOL) {
					/* 3 more words */
					for (j = 0; j < 3; j++)
						CSR_READ_2(sc, WI_DATA1);
				}
				/* unswap 8 bit data fields: */
				for(j=0;j<sizeof(ap.wi_bssid)/2;j++)
					LE16TOH(((u_int16_t *)&ap.wi_bssid[0])[j]);
				for(j=0;j<sizeof(ap.wi_name)/2;j++)
					LE16TOH(((u_int16_t *)&ap.wi_name[0])[j]);
				sc->wi_aps[i].scanreason = ap2_header.wi_reason;
				memcpy(sc->wi_aps[i].bssid, ap2.wi_bssid, 6);
				sc->wi_aps[i].channel = ap2.wi_chid;
				sc->wi_aps[i].signal  = ap2.wi_signal;
				sc->wi_aps[i].noise   = ap2.wi_noise;
				sc->wi_aps[i].quality = ap2.wi_signal - ap2.wi_noise;
				sc->wi_aps[i].capinfo = ap2.wi_capinfo;
				sc->wi_aps[i].interval = ap2.wi_interval;
				sc->wi_aps[i].rate    = ap2.wi_rate;
				if (ap2.wi_namelen > 32)
					ap2.wi_namelen = 32;
				sc->wi_aps[i].namelen = ap2.wi_namelen;
				memcpy(sc->wi_aps[i].name, ap2.wi_name,
				       ap2.wi_namelen);
			}
			break;

		case WI_LUCENT:
			naps = 2 * gen.wi_len / sizeof(ap);
			naps = naps > MAXAPINFO ? MAXAPINFO : naps;
			sc->wi_naps = naps;
			/* Read Data*/
			for (i=0; i < naps; i++) {
				for(j=0; j < sizeof(ap) / 2; j++)
					((u_int16_t *)&ap)[j] =
						CSR_READ_2(sc, WI_DATA1);
				/* unswap 8 bit data fields: */
				for(j=0;j<sizeof(ap.wi_bssid)/2;j++)
					HTOLE16(((u_int16_t *)&ap.wi_bssid[0])[j]);
				for(j=0;j<sizeof(ap.wi_name)/2;j++)
					HTOLE16(((u_int16_t *)&ap.wi_name[0])[j]);
				memcpy(sc->wi_aps[i].bssid, ap.wi_bssid, 6);
				sc->wi_aps[i].channel = ap.wi_chid;
				sc->wi_aps[i].signal  = ap.wi_signal;
				sc->wi_aps[i].noise   = ap.wi_noise;
				sc->wi_aps[i].quality = ap.wi_signal - ap.wi_noise;
				sc->wi_aps[i].capinfo = ap.wi_capinfo;
				sc->wi_aps[i].interval = ap.wi_interval;
				if (ap.wi_namelen > 32)
					ap.wi_namelen = 32;
				sc->wi_aps[i].namelen = ap.wi_namelen;
				memcpy(sc->wi_aps[i].name, ap.wi_name,
				       ap.wi_namelen);
			}
			break;
		}
		/* Done scanning */
		sc->wi_scanning = 0;
		break;

	case WI_INFO_COUNTERS:
		/* some card versions have a larger stats structure */
		len = (gen.wi_len - 1 < sizeof(sc->wi_stats) / 4) ?
			gen.wi_len - 1 : sizeof(sc->wi_stats) / 4;
		ptr = (u_int32_t *)&sc->wi_stats;

		for (i = 0; i < len; i++) {
			t = CSR_READ_2(sc, WI_DATA1);
#ifdef WI_HERMES_STATS_WAR
			if (t > 0xF000)
				t = ~t & 0xFFFF;
#endif
			ptr[i] += t;
		}

		ifp->if_collisions = sc->wi_stats.wi_tx_single_retries +
			sc->wi_stats.wi_tx_multi_retries +
			sc->wi_stats.wi_tx_retry_limit;
		break;

	case WI_INFO_LINK_STAT: {
		static char *msg[] = {
			"connected",
			"disconnected",
			"AP change",
			"AP out of range",
			"AP in range",
			"association failed",
			"unknown link status indication"
		};

		if (gen.wi_len != 2) {
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: WI_INFO_LINK_STAT: len %d\n",
				    sc->sc_dev.dv_xname, gen.wi_len);
			break;
		}
		t = CSR_READ_2(sc, WI_DATA1);

		switch (t) {
		case 6: /* association failed */
			if (!(sc->wi_flags & WI_FLAGS_JOINING))
				break;
			
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: failed JOIN %s\n",
				    sc->sc_dev.dv_xname,
				    ether_sprintf(sc->wi_join_bssid));

			bzero(&sc->wi_join_bssid,
			    sizeof(sc->wi_join_bssid));
			sc->wi_join_channel = 0;
			sc->wi_flags &= ~WI_FLAGS_JOINING;
			break;
		case 2: /* disconnected */
			if (!(sc->wi_flags & WI_FLAGS_CONNECTED))
				break;
			sc->wi_flags &=
			    ~(WI_FLAGS_CONNECTED | WI_FLAGS_AP_IN_RANGE);
			bzero(&sc->wi_current_bssid,
			    sizeof(sc->wi_current_bssid));
			sc->wi_current_channel = 0;
			break;
		case 1: /* connected */
		case 3: /* AP change */

			sc->wi_flags &=
			    ~(WI_FLAGS_CONNECTED | WI_FLAGS_AP_IN_RANGE);

			wreq.wi_type = WI_RID_CURRENT_BSSID;
			wreq.wi_len = WI_MAX_DATALEN;

			if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq) ||
			    wreq.wi_len != 4)
				break;

			(void)memcpy(&sc->wi_current_bssid, &wreq.wi_val[0],
			    sizeof(sc->wi_current_bssid));

			wreq.wi_type = WI_RID_CURRENT_CHAN;
			wreq.wi_len = WI_MAX_DATALEN;

			if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq) ||
			    wreq.wi_len != 2)
				break;

			sc->wi_current_channel = le16toh(wreq.wi_val[0]);

			/* The only Connected/AP Change indication that
			 * ends a JOIN-pending condition comes with the
			 * same channel/BSSID as we asked to JOIN.
			 */
			if ((sc->wi_flags & WI_FLAGS_JOINING) &&
			    (memcmp(&sc->wi_current_bssid, &sc->wi_join_bssid,
			           sizeof(sc->wi_join_bssid)) != 0 ||
			    sc->wi_current_channel != sc->wi_join_channel))
				break;

			sc->wi_flags |=
			    WI_FLAGS_CONNECTED | WI_FLAGS_AP_IN_RANGE;

			sc->wi_flags &= ~WI_FLAGS_JOINING;

#if 0
			wreq.wi_type = WI_RID_COMMQUAL;
			wreq.wi_len = WI_MAX_DATALEN;
			if (wi_read_record(sc, (struct wi_ltv_gen*)&wreq) == 0
			&&  wreq.wi_val[0] == 0)
				sc->wi_flags &= ~WI_FLAGS_AP_IN_RANGE;
#endif
			break;

		case 4: /* AP out of range */
			sc->wi_flags &= ~WI_FLAGS_AP_IN_RANGE;
			if (sc->sc_firmware_type == WI_SYMBOL) {
				wi_cmd(sc, WI_CMD_INQUIRE,
				    WI_INFO_HOST_SCAN_RESULTS, 0, 0);
				break;
			}
			break;

		case 5: /* AP in range */ 
			sc->wi_flags |= WI_FLAGS_AP_IN_RANGE;
			break;
		default:
			t = sizeof(msg) / sizeof(msg[0]) - 1;
			break;
		}
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: %s, BSSID %s %d\n", sc->sc_dev.dv_xname,
			    msg[t - 1], ether_sprintf(sc->wi_current_bssid),
			    sc->wi_current_channel);
		}
		break;

	case WI_INFO_ASSOC_STAT: {
		static char *msg[] = {
			"STA Associated",
			"STA Reassociated",
			"STA Disassociated",
			"Association Failure",
			"Authentication Failed"
		};
		if (gen.wi_len != 10)
                        break;
		for (i=0; i < gen.wi_len - 1; i++)
			((u_int16_t *)&assoc)[i] = CSR_READ_2(sc, WI_DATA1);
		/* unswap 8 bit data fields: */
		for(j=0;j<sizeof(assoc.wi_assoc_sta)/2;j++)
			HTOLE16(((u_int16_t *)&assoc.wi_assoc_sta[0])[j]);
		for(j=0;j<sizeof(assoc.wi_assoc_osta)/2;j++)
			HTOLE16(((u_int16_t *)&assoc.wi_assoc_osta[0])[j]);
		switch (assoc.wi_assoc_stat) {
		case ASSOC:
		case DISASSOC:
		case ASSOCFAIL:
		case AUTHFAIL:
			printf("%s: %s, AP = %02x:%02x:%02x:%02x:%02x:%02x\n",
				sc->sc_dev.dv_xname,
				msg[assoc.wi_assoc_stat - 1],
				assoc.wi_assoc_sta[0]&0xff, assoc.wi_assoc_sta[1]&0xff,
				assoc.wi_assoc_sta[2]&0xff, assoc.wi_assoc_sta[3]&0xff,
				assoc.wi_assoc_sta[4]&0xff, assoc.wi_assoc_sta[5]&0xff);
			break;
		case REASSOC:
			printf("%s: %s, AP = %02x:%02x:%02x:%02x:%02x:%02x, "
				"OldAP = %02x:%02x:%02x:%02x:%02x:%02x\n",
				sc->sc_dev.dv_xname, msg[assoc.wi_assoc_stat - 1],
				assoc.wi_assoc_sta[0]&0xff, assoc.wi_assoc_sta[1]&0xff,
				assoc.wi_assoc_sta[2]&0xff, assoc.wi_assoc_sta[3]&0xff,
				assoc.wi_assoc_sta[4]&0xff, assoc.wi_assoc_sta[5]&0xff,
				assoc.wi_assoc_osta[0]&0xff, assoc.wi_assoc_osta[1]&0xff,
				assoc.wi_assoc_osta[2]&0xff, assoc.wi_assoc_osta[3]&0xff,
				assoc.wi_assoc_osta[4]&0xff, assoc.wi_assoc_osta[5]&0xff);
			break;
		}
		}

	default:
#ifdef WI_DEBUG
		printf("%s: got info type: 0x%04x len=0x%04x\n",
      sc->sc_dev.dv_xname, gen.wi_type,gen.wi_len);
#endif
#if 0
		for (i = 0; i < gen.wi_len; i++) {
			t = CSR_READ_2(sc, WI_DATA1);
			printf("[0x%02x] = 0x%04x\n", i, t);
		}
#endif
		break;
	}
}

static int
wi_join_bss(sc)
	struct wi_softc	*sc;
{
	struct wi_req		wreq;

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
		return 0;
	}

	if (memcmp(&sc->wi_join_bssid, &zero_bssid, sizeof(zero_bssid)) == 0 &&
	    sc->wi_join_channel == 0 && (sc->wi_flags & WI_FLAGS_CONNECTED)) {

		return 0;
	} else if (memcmp(&sc->wi_join_bssid, &sc->wi_current_bssid,
	                  sizeof(sc->wi_join_bssid)) == 0 &&
	           sc->wi_join_channel == sc->wi_current_channel) {

		bzero(&sc->wi_join_bssid, sizeof(sc->wi_join_bssid));
		sc->wi_join_channel = 0;
		return 0;
	}

	sc->wi_flags &= ~(WI_FLAGS_CONNECTED | WI_FLAGS_AP_IN_RANGE);

	/* Indications are unavailable/unreliable in IBSS mode, so
	 * we do not tie things up by setting WI_FLAGS_JOINING.
	 */
	switch (sc->wi_ptype) {
		case WI_PORTTYPE_ADHOC:
		case WI_PORTTYPE_IBSS:
			sc->wi_flags &= ~WI_FLAGS_JOINING;
			break;
		default:
			sc->wi_flags |= WI_FLAGS_JOINING;
	}

	wreq.wi_type = WI_RID_JOIN_REQ;
	wreq.wi_len = 5;
	(void)memcpy(&wreq.wi_val[0], &sc->wi_join_bssid,
	    sizeof(sc->wi_join_bssid));
	wreq.wi_val[3] = htole16(sc->wi_join_channel);

	return wi_write_record(sc, (struct wi_ltv_gen *) &wreq);
}

int wi_intr(arg)
	void *arg;
{
	struct wi_softc		*sc = arg;
	struct ifnet		*ifp;
	u_int16_t		status;
	u_int16_t		last_status, raw_status;
	int s;
	struct timeval		start_time, present_time;

	if (sc->sc_enabled == 0 ||
	    (sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 ||
	    (sc->sc_ethercom.ec_if.if_flags & IFF_RUNNING) == 0)
		return (0);

	ifp = &sc->sc_ethercom.ec_if;

	if (!(ifp->if_flags & IFF_UP)) {
		CSR_WRITE_2(sc, WI_INT_EN, 0);
		CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);
		return 1;
	}

	last_status = 0;

	s = splclock();
	start_time = mono_time;
	splx(s);

	for (;;) {

		/* Only believe a status bit when we enter wi_intr, or when
		 * the bit was "off" the last time through the loop. This is
		 * my strategy to avoid racing the hardware/firmware if I
		 * can re-read the event status register more quickly than
		 * it is updated.
		 */
		raw_status = CSR_READ_2(sc, WI_EVENT_STAT);
		status = raw_status & ~last_status;
		last_status = raw_status & WI_INTRS;

		if (!(raw_status & WI_INTRS)) {
			break;
		}

		if (status & WI_EV_RX) {
			wi_rxeof(sc);
		}

		if (status & WI_EV_TX) {
			wi_txeof(sc, status);
		}

		if (status & WI_EV_ALLOC) {
			int			id;
			id = CSR_READ_2(sc, WI_ALLOC_FID);
			if (id == sc->wi_tx_data_id)
				wi_txeof(sc, status);
		}

		if (status & WI_EV_INFO) {
			wi_update_stats(sc);
		}

		if (status & WI_EV_TX_EXC) {
			wi_txeof(sc, status);
		}

		/* Remember that we no longer disable interrupts. We ack
		 * LAST so that we're not racing against new events to
		 * process the present events. It is bad to lose the
		 * race because an RX/TX buffer is eligible for re-use once
		 * we ack.  Possibly I have seen RX frames too long because
		 * the NIC clobbered the frame-length before I read it all?
		 */
		CSR_WRITE_2(sc, WI_EVENT_ACK, status);

		s = splclock();
		present_time = mono_time;
		splx(s);

		while (present_time.tv_sec > start_time.tv_sec) {
			present_time.tv_usec += 1000000;
			present_time.tv_sec--;
		}
		if (present_time.tv_usec - start_time.tv_usec >= 4000 /*4ms*/) {
			break;
		}
	}

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		wi_start(ifp);

	return 1;
}

/* Must be called at proper protection level! */
static int
wi_cmd(sc, cmd, val0, val1, val2)
	struct wi_softc		*sc;
	int			cmd;
	int			val0;
	int			val1;
	int			val2;
{
	int			i, s = 0;

	/* Wait 100us for the busy bit to clear. */
	for (i = 10; i--; DELAY(10)) {
		if (!(CSR_READ_2(sc, WI_COMMAND) & WI_CMD_BUSY))
			break;
	}

	if (i < 0) {
		printf("%s: wi_cmd: BUSY did not clear, cmd=0x%x\n",
		    sc->sc_dev.dv_xname, cmd);
		return EIO;
	}

	CSR_WRITE_2(sc, WI_PARAM0, val0);
	CSR_WRITE_2(sc, WI_PARAM1, val1);
	CSR_WRITE_2(sc, WI_PARAM2, val2);
	CSR_WRITE_2(sc, WI_COMMAND, cmd);

	/* wait .4 second for the command to complete. study the
	 * distribution of times.
	 */
	for (i = 2000; i--; DELAY(200)) {
		/*
		 * Wait for 'command complete' bit to be
		 * set in the event status register.
		 */
		s = CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_CMD;
		if (s) {
			/* Ack the event and read result code. */
			s = CSR_READ_2(sc, WI_STATUS);
			CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_CMD);
#ifdef foo
			if ((s & WI_CMD_CODE_MASK) != (cmd & WI_CMD_CODE_MASK))
				return(EIO);
#endif
			if (s & WI_STAT_CMD_RESULT)
				return(EIO);
			break;
		}
	}

	if (i < 0) {
		if (!sc->wi_scanning)
			printf("%s: command timed out, cmd=0x%x\n",
				sc->sc_dev.dv_xname, cmd);
		return(ETIMEDOUT);
	}

	return(0);
}

static void
wi_reset(sc)
	struct wi_softc		*sc;
{
	int i;

	sc->wi_flags &=
	    ~(WI_FLAGS_INITIALIZED | WI_FLAGS_CONNECTED | WI_FLAGS_AP_IN_RANGE);

	bzero(&sc->wi_current_bssid, sizeof(sc->wi_current_bssid));
	bzero(&sc->wi_stats, sizeof(sc->wi_stats));

	for (i = 5; i--; DELAY(5 * 1000)) {
		if (wi_cmd(sc, WI_CMD_INI, 0, 0, 0) == 0)
			break;
	}
	
	if (i < 0)
		printf("%s" ": init failed\n", sc->sc_dev.dv_xname);
	else
		sc->wi_flags |= WI_FLAGS_INITIALIZED;

	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);

	/* Calibrate timer. */
	WI_SETVAL(WI_RID_TICK_TIME, 8);

	return;
}

/*
 * Read an LTV record from the NIC.
 */
static int wi_read_record(sc, ltv)
	struct wi_softc		*sc;
	struct wi_ltv_gen	*ltv;
{
	u_int16_t		*ptr;
	int			len, code;
	struct wi_ltv_gen	*oltv, p2ltv;

	if (sc->sc_firmware_type != WI_LUCENT) {
		oltv = ltv;
		switch (ltv->wi_type) {
		case WI_RID_ENCRYPTION:
			p2ltv.wi_type = WI_RID_P2_ENCRYPTION;
			p2ltv.wi_len = 2;
			ltv = &p2ltv;
			break;
		case WI_RID_TX_CRYPT_KEY:
			p2ltv.wi_type = WI_RID_P2_TX_CRYPT_KEY;
			p2ltv.wi_len = 2;
			ltv = &p2ltv;
			break;
		}
	}

	/* Tell the NIC to enter record read mode. */
	if (wi_cmd(sc, WI_CMD_ACCESS|WI_ACCESS_READ, ltv->wi_type, 0, 0))
		return(EIO);

	/* Seek to the record. */
	if (wi_seek(sc, ltv->wi_type, 0, WI_BAP1))
		return(EIO);

	/*
	 * Read the length and record type and make sure they
	 * match what we expect (this verifies that we have enough
	 * room to hold all of the returned data).
	 */
	len = CSR_READ_2(sc, WI_DATA1);
	if (len > ltv->wi_len)
		return(ENOSPC);
	code = CSR_READ_2(sc, WI_DATA1);
	if (code != ltv->wi_type)
		return(EIO);

	ltv->wi_len = len;
	ltv->wi_type = code;

	/* A 2us delay here prevents a MCHK exception on G4 Powerbook.
	 * Go figure.
	 */

	/* Now read the data. */
	ptr = &ltv->wi_val;
	if (ltv->wi_len > 1)
		CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr, ltv->wi_len - 1);

	if (ltv->wi_type == WI_RID_PORTTYPE &&
	    sc->wi_ptype == WI_PORTTYPE_IBSS &&
	    ltv->wi_val == sc->wi_ibss_port) {
		/*
		 * Convert vendor IBSS port type to WI_PORTTYPE_IBSS.
		 * Since Lucent uses port type 1 for BSS *and* IBSS we
		 * have to rely on wi_ptype to distinguish this for us.
		 */
		ltv->wi_val = htole16(WI_PORTTYPE_IBSS);
	} else if (sc->sc_firmware_type != WI_LUCENT) {
		int v;

		switch (oltv->wi_type) {
		case WI_RID_TX_RATE:
		case WI_RID_CUR_TX_RATE:
			switch (le16toh(ltv->wi_val)) {
			case 1: v = 1; break;
			case 2: v = 2; break;
			case 3:	v = 6; break;
			case 4: v = 5; break;
			case 7: v = 7; break;
			case 8: v = 11; break;
			case 15: v = 3; break;
			default: v = 0x100 + le16toh(ltv->wi_val); break;
			}
			oltv->wi_val = htole16(v);
			break;
		case WI_RID_ENCRYPTION:
			oltv->wi_len = 2;
			if (le16toh(ltv->wi_val) & 0x01)
				oltv->wi_val = htole16(1);
			else
				oltv->wi_val = htole16(0);
			break;
		case WI_RID_TX_CRYPT_KEY:
			oltv->wi_len = 2;
			oltv->wi_val = ltv->wi_val;
			break;
		case WI_RID_CNFAUTHMODE:
			oltv->wi_len = 2;
			if (le16toh(ltv->wi_val) & 0x01)
				oltv->wi_val = htole16(1);
			else if (le16toh(ltv->wi_val) & 0x02)
				oltv->wi_val = htole16(2);
			break;
		}
	}

	return(0);
}

/*
 * Same as read, except we inject data instead of reading it.
 */
static int wi_write_record(sc, ltv)
	struct wi_softc		*sc;
	struct wi_ltv_gen	*ltv;
{
	u_int16_t		*ptr;
	int			i;
	struct wi_ltv_gen	p2ltv;

	if (ltv->wi_type == WI_RID_PORTTYPE &&
	    ltv->wi_val == le16toh(WI_PORTTYPE_IBSS)) {
		/* Convert WI_PORTTYPE_IBSS to vendor IBSS port type. */
		p2ltv.wi_type = WI_RID_PORTTYPE;
		p2ltv.wi_len = 2;
		p2ltv.wi_val = sc->wi_ibss_port;
		ltv = &p2ltv;
	} else if (sc->sc_firmware_type != WI_LUCENT) {
		int v;

		switch (ltv->wi_type) {
		case WI_RID_TX_RATE:
			p2ltv.wi_type = WI_RID_TX_RATE;
			p2ltv.wi_len = 2;
			switch (le16toh(ltv->wi_val)) {
			case 1: v = 1; break;
			case 2: v = 2; break;
			case 3:	v = 15; break;
			case 5: v = 4; break;
			case 6: v = 3; break;
			case 7: v = 7; break;
			case 11: v = 8; break;
			default: return EINVAL;
			}
			p2ltv.wi_val = htole16(v);
			ltv = &p2ltv;
			break;
		case WI_RID_ENCRYPTION:
			p2ltv.wi_type = WI_RID_P2_ENCRYPTION;
			p2ltv.wi_len = 2;
			if (le16toh(ltv->wi_val)) {
				uint16_t val = PRIVACY_INVOKED;
				/*
				 * If using shared key WEP we must set the
				 * EXCLUDE_UNENCRYPTED bit.  Symbol cards
				 * need this bit even when not using shared
				 * key.  We can't just test for
				 * IEEE80211_AUTH_SHARED since Symbol cards
				 * have 2 shared key modes.
				 */
				if (sc->wi_authtype != IEEE80211_AUTH_OPEN ||
				    sc->sc_firmware_type == WI_SYMBOL)
					val |= EXCLUDE_UNENCRYPTED;
				/* Tx encryption is broken in Host-AP mode. */
				if (sc->wi_ptype == WI_PORTTYPE_HOSTAP)
					val |= HOST_ENCRYPT;
				p2ltv.wi_val = htole16(val);
			} else
				p2ltv.wi_val =
				    htole16(HOST_ENCRYPT | HOST_DECRYPT);
			ltv = &p2ltv;
			break;
		case WI_RID_TX_CRYPT_KEY:
			p2ltv.wi_type = WI_RID_P2_TX_CRYPT_KEY;
			p2ltv.wi_len = 2;
			p2ltv.wi_val = ltv->wi_val;
			ltv = &p2ltv;
			break;
		case WI_RID_DEFLT_CRYPT_KEYS:
		    {
			int error;
			int keylen;
			struct wi_ltv_str	ws;
			struct wi_ltv_keys	*wk = (struct wi_ltv_keys *)ltv;

			keylen = le16toh(wk->wi_keys[sc->wi_tx_key].wi_keylen);

			for (i = 0; i < 4; i++) {
				memset(&ws, 0, sizeof(ws));
				ws.wi_len = (keylen > 5) ? 8 : 4;
				ws.wi_type = WI_RID_P2_CRYPT_KEY0 + i;
				memcpy(ws.wi_str,
					&wk->wi_keys[i].wi_keydat, keylen);
				error = wi_write_record(sc,
					(struct wi_ltv_gen *)&ws);
				if (error)
					return error;
			}
			return 0;
		    }
		case WI_RID_CNFAUTHMODE:
			p2ltv.wi_type = WI_RID_CNFAUTHMODE;
			p2ltv.wi_len = 2;
			if (le16toh(ltv->wi_val) == 1)
				p2ltv.wi_val = htole16(0x01);
			else if (le16toh(ltv->wi_val) == 2)
				p2ltv.wi_val = htole16(0x02);
			ltv = &p2ltv;
			break;
		}
	}

	if (wi_seek(sc, ltv->wi_type, 0, WI_BAP1))
		return(EIO);

	CSR_WRITE_2(sc, WI_DATA1, ltv->wi_len);
	CSR_WRITE_2(sc, WI_DATA1, ltv->wi_type);

	/* Write data */
	ptr = &ltv->wi_val;
	if (ltv->wi_len > 1)
		CSR_WRITE_MULTI_STREAM_2(sc, WI_DATA1, ptr, ltv->wi_len - 1);

	if (wi_cmd(sc, WI_CMD_ACCESS|WI_ACCESS_WRITE, ltv->wi_type, 0, 0))
		return(EIO);

	return(0);
}

static int wi_seek(sc, id, off, chan)
	struct wi_softc		*sc;
	int			id, off, chan;
{
	int			i;
	int			selreg, offreg;
	int			status;

#if defined(OPTIMIZE_RW_DATA)
	if (sc->wi_last_chan == chan && sc->wi_last_id == id &&
	    sc->wi_last_offset <= off) {
		while (off - sc->wi_last_offset >= len) {
			CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr, len);
			sc->wi_last_offset += len;
		}
		if (sc->wi_last_offset < off) {
			CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr,
			    off - sc->wi_last_offset);
			sc->wi_last_offset = off;
		}
		return 0;
	}
#endif
	switch (chan) {
	case WI_BAP0:
		selreg = WI_SEL0;
		offreg = WI_OFF0;
		break;
	case WI_BAP1:
		selreg = WI_SEL1;
		offreg = WI_OFF1;
		break;
	default:
		printf("%s: invalid data path: %x\n",
		    sc->sc_dev.dv_xname, chan);
		return(EIO);
	}

	CSR_WRITE_2(sc, selreg, id);
	CSR_WRITE_2(sc, offreg, off);

	for (i = WI_TIMEOUT; i--; DELAY(10))
		if (!(CSR_READ_2(sc, offreg) & (WI_OFF_BUSY|WI_OFF_ERR)))
			break;

	if (i < 0) {
		printf("%s: timeout in wi_seek to %x/%x; last status %x\n",
		       sc->sc_dev.dv_xname, id, off, status);
		return(ETIMEDOUT);
	}
#if defined(OPTIMIZE_RW_DATA)
	sc->wi_last_chan = chan;
	sc->wi_last_id = id;
	sc->wi_last_offset = off;
#endif
	return(0);
}

/* buf must be aligned on a u_int16_t boundary. */
static int wi_read_data(sc, id, off, buf, len)
	struct wi_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	u_int16_t		*ptr;

	if (len <= 0)
		return 0;

	if (len & 1) 
		len++;

	if (wi_seek(sc, id, off, WI_BAP1))
		return(EIO);

	ptr = (u_int16_t *)buf;
	CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr, len / 2);

#if defined(OPTIMIZE_RW_DATA)
	sc->wi_last_chan = WI_BAP1;
	sc->wi_last_id = id;
	sc->wi_last_offset = off + len;
#endif
	return(0);
}

/*
 * According to the comments in the HCF Light code, there is a bug in
 * the Hermes (or possibly in certain Hermes firmware revisions) where
 * the chip's internal autoincrement counter gets thrown off during
 * data writes: the autoincrement is missed, causing one data word to
 * be overwritten and subsequent words to be written to the wrong memory
 * locations. The end result is that we could end up transmitting bogus
 * frames without realizing it. The workaround for this is to write a
 * couple of extra guard words after the end of the transfer, then
 * attempt to read then back. If we fail to locate the guard words where
 * we expect them, we preform the transfer over again.
 *
 * buf must be aligned on a u_int16_t boundary.
 */
static int wi_write_data(sc, id, off, buf, len)
	struct wi_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	u_int16_t		*ptr;

	if (len <= 0)
		return 0;

	if (len & 1)
		len++;

#if !defined(OPTIMIZE_RW_DATA) && defined(WI_HERMES_AUTOINC_WAR)
again:
#endif

	if (wi_seek(sc, id, off, WI_BAP0))
		return(EIO);

	ptr = (u_int16_t *)buf;
	CSR_WRITE_MULTI_STREAM_2(sc, WI_DATA0, ptr, len / 2);

#if !defined(OPTIMIZE_RW_DATA) && defined(WI_HERMES_AUTOINC_WAR)
	CSR_WRITE_2(sc, WI_DATA0, 0x1234);
	CSR_WRITE_2(sc, WI_DATA0, 0x5678);

	if (wi_seek(sc, id, off + len, WI_BAP0))
		return(EIO);

	if (CSR_READ_2(sc, WI_DATA0) != 0x1234 ||
	    CSR_READ_2(sc, WI_DATA0) != 0x5678) {
		if (sc->sc_ethercom.ec_if.if_flags & IFF_DEBUG)
			printf("%s: auto-inc error\n", sc->sc_dev.dv_xname);
		goto again;
	}
#endif

#if defined(OPTIMIZE_RW_DATA)
	sc->wi_last_chan = WI_BAP0;
	sc->wi_last_id = id;
	sc->wi_last_offset = off + len;
#endif
	return(0);
}

/*
 * Allocate a region of memory inside the NIC and zero
 * it out.
 */
static int wi_alloc_nicmem(sc, len, id)
	struct wi_softc		*sc;
	int			len;
	int			*id;
{
	int			i;

#if defined(OPTIMIZE_RW_DATA)
	wi_rewind(sc);
#endif

	if (wi_cmd(sc, WI_CMD_ALLOC_MEM, len, 0, 0)) {
		printf("%s: failed to allocate %d bytes on NIC\n",
		    sc->sc_dev.dv_xname, len);
		return(ENOMEM);
	}

	for (i = WI_TIMEOUT; i--; DELAY(10)) {
		if (CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_ALLOC)
			break;
	}

	if (i < 0) {
		printf("%s: TIMED OUT in alloc\n", sc->sc_dev.dv_xname);
		return(ETIMEDOUT);
	}

	*id = CSR_READ_2(sc, WI_ALLOC_FID);
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_ALLOC);

	if (wi_seek(sc, *id, 0, WI_BAP0)) {
		printf("%s: seek failed in alloc\n", sc->sc_dev.dv_xname);
		return(EIO);
	}

	for (i = 0; i < len / 2; i++)
		CSR_WRITE_2(sc, WI_DATA0, 0);

	return(0);
}

static void wi_setmulti(sc)
	struct wi_softc		*sc;
{
	struct ifnet		*ifp;
	int			i = 0;
	struct wi_ltv_mcast	mcast;
	struct ether_multi *enm;
	struct ether_multistep estep;
	struct ethercom *ec = &sc->sc_ethercom;

	ifp = &sc->sc_ethercom.ec_if;

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		memset((char *)&mcast, 0, sizeof(mcast));
		mcast.wi_type = WI_RID_MCAST_LIST;
		mcast.wi_len = ((ETHER_ADDR_LEN / 2) * 16) + 1;

		wi_write_record(sc, (struct wi_ltv_gen *)&mcast);
		return;
	}

	i = 0;
	ETHER_FIRST_MULTI(estep, ec, enm);
	while (enm != NULL) {
		/* Punt on ranges or too many multicast addresses. */
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0 ||
		    i >= 16)
			goto allmulti;

		memcpy((char *)&mcast.wi_mcast[i], enm->enm_addrlo,
		    ETHER_ADDR_LEN);
		i++;
		ETHER_NEXT_MULTI(estep, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	mcast.wi_type = WI_RID_MCAST_LIST;
	mcast.wi_len = ((ETHER_ADDR_LEN / 2) * i) + 1;
	wi_write_record(sc, (struct wi_ltv_gen *)&mcast);
}

static int
wi_setdef(sc, wreq)
	struct wi_softc		*sc;
	struct wi_req		*wreq;
{
	struct sockaddr_dl	*sdl;
	struct ifnet		*ifp;
	int error = 0;

	ifp = &sc->sc_ethercom.ec_if;

	switch(wreq->wi_type) {
	case WI_RID_MAC_NODE:
		sdl = (struct sockaddr_dl *)ifp->if_sadl;
		memcpy((char *)&sc->sc_macaddr, (char *)&wreq->wi_val,
		    ETHER_ADDR_LEN);
		memcpy(LLADDR(sdl), (char *)&wreq->wi_val, ETHER_ADDR_LEN);
		break;
	case WI_RID_PORTTYPE:
		error = wi_sync_media(sc, le16toh(wreq->wi_val[0]),
		    sc->wi_tx_rate);
		break;
	case WI_RID_TX_RATE:
		error = wi_sync_media(sc, sc->wi_ptype,
		    le16toh(wreq->wi_val[0]));
		break;
	case WI_RID_MAX_DATALEN:
		sc->wi_max_data_len = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_RTS_THRESH:
		sc->wi_rts_thresh = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_SYSTEM_SCALE:
		sc->wi_ap_density = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_CREATE_IBSS:
		sc->wi_create_ibss = le16toh(wreq->wi_val[0]);
		error = wi_sync_media(sc, sc->wi_ptype, sc->wi_tx_rate);
		break;
	case WI_RID_OWN_CHNL:
		sc->wi_create_channel = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_NODENAME:
		error = wi_set_ssid(&sc->wi_nodeid,
		    (u_int8_t *)&wreq->wi_val[1], le16toh(wreq->wi_val[0]));
		break;
	case WI_RID_DESIRED_SSID:
		error = wi_set_ssid(&sc->wi_netid,
		    (u_int8_t *)&wreq->wi_val[1], le16toh(wreq->wi_val[0]));
		break;
	case WI_RID_OWN_SSID:
		error = wi_set_ssid(&sc->wi_ibssid,
		    (u_int8_t *)&wreq->wi_val[1], le16toh(wreq->wi_val[0]));
		break;
	case WI_RID_PM_ENABLED:
		sc->wi_pm_enabled = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_MICROWAVE_OVEN:
		sc->wi_mor_enabled = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_MAX_SLEEP:
		sc->wi_max_sleep = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_CNFAUTHMODE:
		sc->wi_authtype = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_ROAMING_MODE:
		sc->wi_roaming = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_ENCRYPTION:
		sc->wi_use_wep = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_TX_CRYPT_KEY:
		sc->wi_tx_key = le16toh(wreq->wi_val[0]);
		break;
	case WI_RID_DEFLT_CRYPT_KEYS:
		memcpy((char *)&sc->wi_keys, (char *)wreq,
		    sizeof(struct wi_ltv_keys));
		break;
	default:
		error = EINVAL;
		break;
	}

	return (error);
}

static int
wi_getdef(sc, wreq)
	struct wi_softc		*sc;
	struct wi_req		*wreq;
{
	struct sockaddr_dl	*sdl;
	struct ifnet		*ifp;
	int error = 0;

	ifp = &sc->sc_ethercom.ec_if;

	wreq->wi_len = 2;			/* XXX */
	switch (wreq->wi_type) {
	case WI_RID_MAC_NODE:
		wreq->wi_len += ETHER_ADDR_LEN / 2 - 1;
		sdl = (struct sockaddr_dl *)ifp->if_sadl;
		memcpy(&wreq->wi_val, &sc->sc_macaddr, ETHER_ADDR_LEN);
		memcpy(&wreq->wi_val, LLADDR(sdl), ETHER_ADDR_LEN);
		break;
	case WI_RID_PORTTYPE:
		wreq->wi_val[0] = htole16(sc->wi_ptype);
		break;
	case WI_RID_TX_RATE:
		wreq->wi_val[0] = htole16(sc->wi_tx_rate);
		break;
	case WI_RID_MAX_DATALEN:
		wreq->wi_val[0] = htole16(sc->wi_max_data_len);
		break;
	case WI_RID_RTS_THRESH:
		wreq->wi_val[0] = htole16(sc->wi_rts_thresh);
		break;
	case WI_RID_SYSTEM_SCALE:
		wreq->wi_val[0] = htole16(sc->wi_ap_density);
		break;
	case WI_RID_CREATE_IBSS:
		wreq->wi_val[0] = htole16(sc->wi_create_ibss);
		break;
	case WI_RID_OWN_CHNL:
		wreq->wi_val[0] = htole16(sc->wi_create_channel);
		break;
	case WI_RID_NODENAME:
		wi_request_fill_ssid(wreq, &sc->wi_nodeid);
		break;
	case WI_RID_DESIRED_SSID:
		wi_request_fill_ssid(wreq, &sc->wi_netid);
		break;
	case WI_RID_OWN_SSID:
		wi_request_fill_ssid(wreq, &sc->wi_ibssid);
		break;
	case WI_RID_PM_ENABLED:
		wreq->wi_val[0] = htole16(sc->wi_pm_enabled);
		break;
	case WI_RID_MICROWAVE_OVEN:
		wreq->wi_val[0] = htole16(sc->wi_mor_enabled);
		break;
	case WI_RID_MAX_SLEEP:
		wreq->wi_val[0] = htole16(sc->wi_max_sleep);
		break;
	case WI_RID_CNFAUTHMODE:
		wreq->wi_val[0] = htole16(sc->wi_authtype);
		break;
	case WI_RID_ROAMING_MODE:
		wreq->wi_val[0] = htole16(sc->wi_roaming);
		break;
	case WI_RID_WEP_AVAIL:
		wreq->wi_val[0] = (sc->wi_flags & WI_FLAGS_HAS_WEP) ?
		    htole16(1) : htole16(0);
		break;
	case WI_RID_ENCRYPTION:
		wreq->wi_val[0] = htole16(sc->wi_use_wep);
		break;
	case WI_RID_TX_CRYPT_KEY:
		wreq->wi_val[0] = htole16(sc->wi_tx_key);
		break;
	case WI_RID_DEFLT_CRYPT_KEYS:
		wreq->wi_len += sizeof(struct wi_ltv_keys) / 2 - 1;
		memcpy(wreq, &sc->wi_keys, sizeof(struct wi_ltv_keys));
		break;
	default:
#if 0
		error = EIO;
#else
#ifdef WI_DEBUG
		printf("%s: wi_getdef: unknown request %d\n",
		    sc->sc_dev.dv_xname, wreq->wi_type);
#endif
#endif
		break;
	}

	return (error);
}

static int
wi_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	int			s, error = 0;
	int			len;
	struct wi_softc		*sc = ifp->if_softc;
	struct wi_req		wreq;
	struct ifreq		*ifr;
	struct proc		*p = curproc;
	struct ieee80211_nwid	nwid;

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENXIO);

	s = splnet();

	ifr = (struct ifreq *)data;

	if (!sc->sc_attached) {
		splx(s);
		return(ENODEV);
	}
	
	switch(command) {
	case SIOCSWAVELAN:
	case SIOCS80211NWID:
	case SIOCS80211NWKEY:
	case SIOCS80211POWER:
	case SIOCS80211BSSID:
	case SIOCS80211CHANNEL:
		error = suser(p->p_ucred, &p->p_acflag);
		if (error) {
			splx(s);
			return (error);
		}
	default:
		break;
	}

	switch (command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
		error = ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING &&
			    ifp->if_flags & IFF_PROMISC &&
			    !(sc->wi_if_flags & IFF_PROMISC)) {
				if (sc->wi_ptype != WI_PORTTYPE_HOSTAP)
					WI_SETVAL(WI_RID_PROMISC, 1);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC) &&
			    sc->wi_if_flags & IFF_PROMISC) {
				if (sc->wi_ptype != WI_PORTTYPE_HOSTAP)
					WI_SETVAL(WI_RID_PROMISC, 0);
			} else
				wi_init(ifp);
		} else if (ifp->if_flags & IFF_RUNNING)
			wi_stop(ifp, 0);
		sc->wi_if_flags = ifp->if_flags;

		if (!(ifp->if_flags & IFF_UP)) {
			if (sc->sc_enabled) {
				if (sc->sc_disable)
					(*sc->sc_disable)(sc);
				sc->sc_enabled = 0;
				ifp->if_flags &= ~IFF_RUNNING;
			}
		}
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (command == SIOCADDMULTI) ?
			ether_addmulti(ifr, &sc->sc_ethercom) :
			ether_delmulti(ifr, &sc->sc_ethercom);
		if (error == ENETRESET) {
			if (sc->sc_enabled != 0) {
				/*
				 * Multicast list has changed.  Set the
				 * hardware filter accordingly.
				 */
				wi_setmulti(sc);
			}
			error = 0;
		}
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, command);
		break;
	case SIOCGWAVELAN:
		error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
		if (error)
			break;
		if (wreq.wi_len > WI_MAX_DATALEN) {
			error = EINVAL;
			break;
		}
		if (wreq.wi_type == WI_RID_IFACE_STATS) {
			memcpy((char *)&wreq.wi_val, (char *)&sc->wi_stats,
			    sizeof(sc->wi_stats));
			wreq.wi_len = (sizeof(sc->wi_stats) / 2) + 1;
		} else if (wreq.wi_type == WI_RID_READ_APS) {
			if (sc->wi_scanning) {
				error = EINPROGRESS;
				break;
			} else {
				len = sc->wi_naps * sizeof(struct wi_apinfo);
				len = len > WI_MAX_DATALEN ? WI_MAX_DATALEN : len;
				len = len / sizeof(struct wi_apinfo);
				memcpy((char *)&wreq.wi_val, (char *)&len, sizeof(len));
				memcpy((char *)&wreq.wi_val + sizeof(len),
					(char *)&sc->wi_aps,
					len * sizeof(struct wi_apinfo));
			}
		} else if (wreq.wi_type == WI_RID_DEFLT_CRYPT_KEYS) {
			/* For non-root user, return all-zeroes keys */
			if (suser(p->p_ucred, &p->p_acflag))
				memset((char *)&wreq, 0,
				    sizeof(struct wi_ltv_keys));
			else
				memcpy((char *)&wreq, (char *)&sc->wi_keys,
				    sizeof(struct wi_ltv_keys));
		} else {
			if (sc->sc_enabled == 0 ||
			    (wreq.wi_type == WI_RID_ROAMING_MODE &&
			     (sc->wi_flags & WI_FLAGS_HAS_ROAMING) == 0) ||
			    (wreq.wi_type == WI_RID_CREATE_IBSS &&
			     (sc->wi_flags & WI_FLAGS_HAS_CREATE_IBSS) == 0) ||
			    (wreq.wi_type == WI_RID_MICROWAVE_OVEN &&
			     (sc->wi_flags & WI_FLAGS_HAS_MOR) == 0))
				error = wi_getdef(sc, &wreq);
			else if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq))
				error = EINVAL;
		}
		if (error == 0)
			error = copyout(&wreq, ifr->ifr_data, sizeof(wreq));
		break;
	case SIOCSWAVELAN:
		error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
		if (error)
			break;
		error = EINVAL;
		if (wreq.wi_len > WI_MAX_DATALEN)
			break;
		switch (wreq.wi_type) {
		case WI_RID_IFACE_STATS:
			if (sc->sc_enabled)
				wi_inquire(sc);
			break;
		case WI_RID_MGMT_XMIT:
			error = wi_mgmt_xmit(sc, (caddr_t)&wreq.wi_val,
			    wreq.wi_len);
			break;
		case WI_RID_SCAN_APS:
			if (wreq.wi_len != 4) {
				error = EINVAL;
				break;
			}
			if (!sc->wi_scanning) {
				switch (sc->sc_firmware_type) {
				case WI_LUCENT:
					break;
				case WI_INTERSIL:
					wreq.wi_type = WI_RID_SCAN_REQ;
					error = wi_write_record(sc,
					    (struct wi_ltv_gen *)&wreq);
					break;
				case WI_SYMBOL:
					/*
					 * XXX only supported on 3.x ?
					 */
					wreq.wi_type = WI_RID_BCAST_SCAN_REQ;
					wreq.wi_val[0] =
					    BSCAN_BCAST | BSCAN_ONETIME;
					wreq.wi_len = 2;
					error = wi_write_record(sc,
					    (struct wi_ltv_gen *)&wreq);
					break;
				}
				if (!error) {
					sc->wi_scanning = 1;
					callout_reset(&sc->wi_scan_sh, hz * 1,
						wi_wait_scan, sc);
				}
			}
			break;
		default:
			/*
			 * Filter stuff out based on what the
			 * card can do.
			 */
			if ((wreq.wi_type == WI_RID_ROAMING_MODE &&
			     (sc->wi_flags & WI_FLAGS_HAS_ROAMING) == 0) ||
			    (wreq.wi_type == WI_RID_CREATE_IBSS &&
			     (sc->wi_flags & WI_FLAGS_HAS_CREATE_IBSS) == 0) ||
			    (wreq.wi_type == WI_RID_MICROWAVE_OVEN &&
			     (sc->wi_flags & WI_FLAGS_HAS_MOR) == 0))
				break;

			if (wreq.wi_len > WI_MAX_DATALEN)
				error = EINVAL;
			else if (sc->sc_enabled != 0)
				error = wi_write_record(sc,
				    (struct wi_ltv_gen *)&wreq);
			if (error == 0)
				error = wi_setdef(sc, &wreq);
			if (error == 0 && sc->sc_enabled != 0)
				/* Reinitialize WaveLAN. */
				wi_init(ifp);
		}
		break;
	case SIOCS80211BSSID:

		if (sc->wi_ptype != WI_PORTTYPE_IBSS &&
		    sc->wi_ptype != WI_PORTTYPE_BSS) {
			error = EINVAL;
			break;
		}
		/* don't join twice. */
		if (sc->wi_flags & WI_FLAGS_JOINING) {
			error = EAGAIN;
			break;
		}

		(void)memcpy(&sc->wi_join_bssid,
		       &((struct ieee80211_bssid *)data)->i_bssid,
		       sizeof(sc->wi_join_bssid));

		error = wi_join_bss(sc);
		break;
	case SIOCG80211BSSID:
		if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
			(void)memcpy(&((struct ieee80211_bssid *)data)->i_bssid,
			    LLADDR(ifp->if_sadl), IEEE80211_ADDR_LEN);
		} else {
			(void)memcpy(
			    &((struct ieee80211_bssid *)data)->i_bssid,
			    &sc->wi_current_bssid,
			    sizeof(sc->wi_current_bssid));
		}
		break;
	case SIOCS80211CHANNEL:
		error = wi_set_channel(sc, ((struct ieee80211_channel *)data));
		break;
	case SIOCG80211CHANNEL:
		error = wi_get_channel(sc, ((struct ieee80211_channel *)data));
		break;
	case SIOCG80211NWID:
		if (sc->sc_enabled == 0) {
			/* Return the desired ID */
			error = copyout(&sc->wi_netid, ifr->ifr_data,
			    sizeof(sc->wi_netid));
		} else {
			wreq.wi_type = WI_RID_CURRENT_SSID;
			wreq.wi_len = WI_MAX_DATALEN;
			if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq) ||
			    le16toh(wreq.wi_val[0]) > IEEE80211_NWID_LEN)
				error = EINVAL;
			else {
				wi_set_ssid(&nwid, (u_int8_t *)&wreq.wi_val[1],
				    le16toh(wreq.wi_val[0]));
				error = copyout(&nwid, ifr->ifr_data,
				    sizeof(nwid));
			}
		}
		break;
	case SIOCS80211NWID:
		error = copyin(ifr->ifr_data, &nwid, sizeof(nwid));
		if (error != 0)
			break;
		if (nwid.i_len > IEEE80211_NWID_LEN) {
			error = EINVAL;
			break;
		}
		if (sc->wi_netid.i_len == nwid.i_len &&
		    memcmp(sc->wi_netid.i_nwid, nwid.i_nwid, nwid.i_len) == 0)
			break;
		wi_set_ssid(&sc->wi_netid, nwid.i_nwid, nwid.i_len);
		if (sc->sc_enabled != 0)
			/* Reinitialize WaveLAN. */
			wi_init(ifp);
		break;
	case SIOCS80211NWKEY:
		error = wi_set_nwkey(sc, (struct ieee80211_nwkey *)data);
		break;
	case SIOCG80211NWKEY:
		error = wi_get_nwkey(sc, (struct ieee80211_nwkey *)data);
		break;
	case SIOCS80211POWER:
		error = wi_set_pm(sc, (struct ieee80211_power *)data);
		break;
	case SIOCG80211POWER:
		error = wi_get_pm(sc, (struct ieee80211_power *)data);
		break;
	case SIOCHOSTAP_ADD:
	case SIOCHOSTAP_DEL:
	case SIOCHOSTAP_GET:
	case SIOCHOSTAP_GETALL:
	case SIOCHOSTAP_GFLAGS:
	case SIOCHOSTAP_SFLAGS:
		/* Send all Host-AP specific ioctls to the Host-AP code. */
		error = wihap_ioctl(sc, command, data);
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return (error);
}

static int
wi_init(ifp)
	struct ifnet *ifp;
{
	struct wi_softc *sc = ifp->if_softc;
	struct wi_req wreq;
	struct wi_ltv_macaddr mac;
	int s, error, id = 0, wasenabled;

	if (!sc->sc_attached)
		return(ENODEV);

	s = splnet();
	wasenabled = sc->sc_enabled;
	if (!sc->sc_enabled) {
		if ((error = (*sc->sc_enable)(sc)) != 0)
			goto out;
		sc->sc_enabled = 1;
	}

	wi_stop(ifp, 0);
	/* Symbol firmware cannot be initialized more than once */
	if (!(sc->sc_firmware_type == WI_SYMBOL && wasenabled))
		wi_reset(sc);

	/* Program max data length. */
	WI_SETVAL(WI_RID_MAX_DATALEN, sc->wi_max_data_len);

	/* Enable/disable IBSS creation. */
	WI_SETVAL(WI_RID_CREATE_IBSS, sc->wi_create_ibss);

	/* Set the port type. */
	WI_SETVAL(WI_RID_PORTTYPE, sc->wi_ptype);

	/* Program the RTS/CTS threshold. */
	WI_SETVAL(WI_RID_RTS_THRESH, sc->wi_rts_thresh);

	/* Program the TX rate */
	WI_SETVAL(WI_RID_TX_RATE, sc->wi_tx_rate);

	/* Access point density */
	WI_SETVAL(WI_RID_SYSTEM_SCALE, sc->wi_ap_density);

	/* Power Management Enabled */
	WI_SETVAL(WI_RID_PM_ENABLED, sc->wi_pm_enabled);

	/* Power Managment Max Sleep */
	WI_SETVAL(WI_RID_MAX_SLEEP, sc->wi_max_sleep);

	/* Roaming type */
	if (sc->wi_flags & WI_FLAGS_HAS_ROAMING)
		WI_SETVAL(WI_RID_ROAMING_MODE, sc->wi_roaming);

	/* Specify the network name */
	wi_write_ssid(sc, WI_RID_DESIRED_SSID, &wreq, &sc->wi_netid);

	/* Specify the IBSS name */
	if (sc->wi_netid.i_len != 0 &&
	    (sc->wi_ptype == WI_PORTTYPE_HOSTAP ||
	     (sc->wi_create_ibss && sc->wi_ptype == WI_PORTTYPE_IBSS)))
		wi_write_ssid(sc, WI_RID_OWN_SSID, &wreq, &sc->wi_netid);
	else
		wi_write_ssid(sc, WI_RID_OWN_SSID, &wreq, &sc->wi_ibssid);

	/* Specify the frequency to use */
	WI_SETVAL(WI_RID_OWN_CHNL, sc->wi_create_channel);

	/* Program the nodename. */
	wi_write_ssid(sc, WI_RID_NODENAME, &wreq, &sc->wi_nodeid);

	/* Set our MAC address. */
	mac.wi_len = 4;
	mac.wi_type = WI_RID_MAC_NODE;
	memcpy(&mac.wi_mac_addr, sc->sc_macaddr, ETHER_ADDR_LEN);
	wi_write_record(sc, (struct wi_ltv_gen *)&mac);

	/*
	 * Initialize promiscuous mode.
	 *	Being in the Host-AP mode causes a great
	 *	deal of pain if promiscuous mode is set.
	 *	Therefore we avoid confusing the firmware
	 *	and always reset promiscuous mode in Host-AP
	 *	mode.  Host-AP sees all the packets anyway.
	 */
	if (sc->wi_ptype != WI_PORTTYPE_HOSTAP &&
	    (ifp->if_flags & IFF_PROMISC) != 0) {
		WI_SETVAL(WI_RID_PROMISC, 1);
	} else {
		WI_SETVAL(WI_RID_PROMISC, 0);
	}

	/* Configure WEP. */
	if (sc->wi_flags & WI_FLAGS_HAS_WEP) {
		WI_SETVAL(WI_RID_ENCRYPTION, sc->wi_use_wep);
		WI_SETVAL(WI_RID_TX_CRYPT_KEY, sc->wi_tx_key);
		sc->wi_keys.wi_len = (sizeof(struct wi_ltv_keys) / 2) + 1;
		sc->wi_keys.wi_type = WI_RID_DEFLT_CRYPT_KEYS;
		wi_write_record(sc, (struct wi_ltv_gen *)&sc->wi_keys);
		if (sc->sc_firmware_type != WI_LUCENT && sc->wi_use_wep) {
			/*
			 * ONLY HWB3163 EVAL-CARD Firmware version
			 * less than 0.8 variant2
			 *
			 *   If promiscuous mode disable, Prism2 chip
			 *  does not work with WEP .
			 * It is under investigation for details.
			 * (ichiro@netbsd.org)
			 */
			if (sc->sc_firmware_type == WI_INTERSIL &&
			    sc->sc_sta_firmware_ver < 802 ) {
				/* firm ver < 0.8 variant 2 */
				WI_SETVAL(WI_RID_PROMISC, 1);
			}
			WI_SETVAL(WI_RID_CNFAUTHMODE, sc->wi_authtype);
		}
	}

	/* Set multicast filter. */
	wi_setmulti(sc);

	sc->wi_flags &= ~(WI_FLAGS_JOINING | WI_FLAGS_CONNECTED |
	                  WI_FLAGS_AP_IN_RANGE);

	/* Enable desired port */
	wi_cmd(sc, WI_CMD_ENABLE | sc->wi_portnum, 0, 0, 0);

	/*  scanning variable is modal, therefore reinit to OFF, in case it was on. */
	sc->wi_scanning=0;
	sc->wi_naps=0;

	if ((error = wi_alloc_nicmem(sc, WI_TX_BUFSIZE, &id)) != 0) {
		printf("%s: tx buffer allocation failed\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}
	sc->wi_tx_data_id = id;

	if ((error = wi_alloc_nicmem(sc, WI_TX_BUFSIZE, &id)) != 0) {
		printf("%s: mgmt. buffer allocation failed\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}
	sc->wi_tx_mgmt_id = id;

	/* Enable interrupts */
	CSR_WRITE_2(sc, WI_INT_EN, WI_INTRS);

	wihap_init(sc);

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
		splx(s);
		return error;
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_reset(&sc->wi_inquire_ch, hz * 60, wi_inquire, sc);

	splx(s);
	return 0;
}

static const u_int32_t crc32_tab[] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

#define RC4STATE 256
#define RC4KEYLEN 16
#define RC4SWAP(x,y) \
    do { u_int8_t t = state[x]; state[x] = state[y]; state[y] = t; } while(0)

static void
wi_do_hostencrypt(struct wi_softc *sc, caddr_t buf, int len)
{
	u_int32_t i, crc, klen;
	u_int8_t state[RC4STATE], key[RC4KEYLEN];
	u_int8_t x, y, *dat;

	if (!sc->wi_icv_flag) {
		sc->wi_icv = arc4random();
		sc->wi_icv_flag++;
	} else
		sc->wi_icv++;
	/*
	 * Skip 'bad' IVs from Fluhrer/Mantin/Shamir:
	 * (B, 255, N) with 3 <= B < 8
	 */
	if (sc->wi_icv >= 0x03ff00 &&
            (sc->wi_icv & 0xf8ff00) == 0x00ff00)
                sc->wi_icv += 0x000100;

	/* prepend 24bit IV to tx key, byte order does not matter */
	key[0] = sc->wi_icv >> 16;
	key[1] = sc->wi_icv >> 8;
	key[2] = sc->wi_icv;

	klen = le16toh(sc->wi_keys.wi_keys[sc->wi_tx_key].wi_keylen) +
	    IEEE80211_WEP_IVLEN;
	klen = (klen >= RC4KEYLEN) ? RC4KEYLEN : RC4KEYLEN/2;
	bcopy((char *)&sc->wi_keys.wi_keys[sc->wi_tx_key].wi_keydat,
	    (char *)key + IEEE80211_WEP_IVLEN, klen - IEEE80211_WEP_IVLEN);

	/* rc4 keysetup */
	x = y = 0;
	for (i = 0; i < RC4STATE; i++)
		state[i] = i;
	for (i = 0; i < RC4STATE; i++) {
		y = (key[x] + state[i] + y) % RC4STATE;
		RC4SWAP(i, y);
		x = (x + 1) % klen;
	}

	/* output: IV, tx keyid, rc4(data), rc4(crc32(data)) */
	dat = buf;
	dat[0] = key[0];
	dat[1] = key[1];
	dat[2] = key[2];
	dat[3] = sc->wi_tx_key << 6;		/* pad and keyid */
	dat += 4;

	/* compute rc4 over data, crc32 over data */
	crc = ~0;
	x = y = 0;
	for (i = 0; i < len; i++) {
		x = (x + 1) % RC4STATE;
		y = (state[x] + y) % RC4STATE;
		RC4SWAP(x, y);
		crc = crc32_tab[(crc ^ dat[i]) & 0xff] ^ (crc >> 8);
		dat[i] ^= state[(state[x] + state[y]) % RC4STATE];
	}
	crc = ~crc;
	dat += len;

	/* append little-endian crc32 and encrypt */
	dat[0] = crc;
	dat[1] = crc >> 8;
	dat[2] = crc >> 16;
	dat[3] = crc >> 24;
	for (i = 0; i < IEEE80211_WEP_CRCLEN; i++) {
		x = (x + 1) % RC4STATE;
		y = (state[x] + y) % RC4STATE;
		RC4SWAP(x, y);
		dat[i] ^= state[(state[x] + state[y]) % RC4STATE];
	}
}

static void
wi_start(ifp)
	struct ifnet		*ifp;
{
	struct wi_softc		*sc;
	struct mbuf		*m0;
	struct ether_header	*eh;
	int			id;
	int			len;
	struct wi_frame		*hdr;
	struct llc		*llc;
	u_int8_t		*body, *payload;
#if defined(WI_HOSTAP_POWERSAVE)
	int			more_data = 0;
#endif

	sc = ifp->if_softc;

	if (!sc->sc_attached)
		return;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	/* discard packets when disconnected. postpone sending packets when
	 * connected but out of range.
	 */
	if (!(sc->wi_flags & WI_FLAGS_AP_IN_RANGE) &&
	    (sc->wi_flags & WI_FLAGS_CONNECTED)) {
		return;
	}
	hdr = (struct wi_frame *)&sc->wi_txbuf;

	body = (u_int8_t *)(hdr + 1);

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP && sc->wi_use_wep) {
		llc = (struct llc *)(body + IEEE80211_WEP_IVLEN +
		    IEEE80211_WEP_KIDLEN);
	} else {
		llc = (struct llc *)body;
	}

	payload = (u_int8_t *)(llc + 1);

 nextpkt:
#if defined(WI_HOSTAP_POWERSAVE)
	/* power-save queue, first. */
	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP)
		m0 = wihap_dequeue(&sc->wi_hostap_info, &more_data);

	if (m0 == NULL)
#endif /* WI_HOSTAP_POWERSAVE */
		IFQ_DEQUEUE(&ifp->if_snd, m0);

	if (m0 == NULL)
		return;

	bzero(hdr, sizeof(*hdr));
	hdr->wi_frame_ctl = htole16(WI_FTYPE_DATA);
	id = sc->wi_tx_data_id;
	eh = mtod(m0, struct ether_header *);

#if defined(OPTIMIZE_RW_DATA)
	wi_rewind(sc);
#endif

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
		if (wihap_check_tx(sc, m0, &hdr->wi_tx_rate) == 0)
			goto nextpkt;
#if defined(WI_HOSTAP_POWERSAVE)
		if (more_data)
			hdr->wi_frame_ctl |= htole16(WI_FCTL_PM);
#endif /* WI_HOSTAP_POWERSAVE */
	}

	/*
	 * Use RFC1042 encoding for everything.
	 */

	hdr->wi_tx_ctl |= htole16(WI_ENC_TX_802_11);

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP) {
		hdr->wi_frame_ctl |= htole16(WI_FCTL_FROMDS);
		memcpy(&hdr->wi_addr1, &eh->ether_dhost,
		    IEEE80211_ADDR_LEN);
		memcpy(&hdr->wi_addr2, LLADDR(ifp->if_sadl),
		    IEEE80211_ADDR_LEN);
		memcpy((char *)&hdr->wi_addr3,
		    (char *)&eh->ether_shost, IEEE80211_ADDR_LEN);
	} else {
		if (sc->wi_ptype == WI_PORTTYPE_BSS) {
			hdr->wi_frame_ctl |= htole16(WI_FCTL_TODS);
			memcpy(&hdr->wi_addr1, &sc->wi_current_bssid,
			    IEEE80211_ADDR_LEN);
			memcpy(&hdr->wi_addr3, &eh->ether_dhost,
			    IEEE80211_ADDR_LEN);
		} else {
			memcpy(&hdr->wi_addr1, &eh->ether_dhost,
			    IEEE80211_ADDR_LEN);
			memcpy(&hdr->wi_addr3, &sc->wi_current_bssid,
			    IEEE80211_ADDR_LEN);
		}
		memcpy(&hdr->wi_addr2, &eh->ether_shost,
		    IEEE80211_ADDR_LEN);
	}

	len = sizeof(struct llc) - sizeof(struct ether_header) +
	    m0->m_pkthdr.len;

	llc->llc_dsap = llc->llc_ssap = LLC_SNAP_LSAP;
	llc->llc_control = LLC_UI;
	llc->llc_snap.org_code[0] = 0;
	llc->llc_snap.org_code[1] = 0;
	llc->llc_snap.org_code[2] = 0;
	llc->llc_snap.ether_type = eh->ether_type;

	m_copydata(m0, sizeof(struct ether_header),
	    m0->m_pkthdr.len - sizeof(struct ether_header),
	    payload);

	if (sc->wi_ptype == WI_PORTTYPE_HOSTAP && sc->wi_use_wep) {

		hdr->wi_frame_ctl |= htole16(WI_FCTL_WEP);

		wi_do_hostencrypt(sc, body, len);

		len += IEEE80211_WEP_IVLEN +
		    IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN;
	}

	hdr->wi_dat_len = htole16(len);

	wi_write_data(sc, id, 0, (caddr_t)&sc->wi_txbuf,
	    sizeof(struct wi_frame) + len);

#if NBPFILTER > 0
	/*
	 * If there's a BPF listener, bounce a copy of
	 * this frame to him.
	 */
	wi_tap_802_3(sc, m0, hdr);
#endif

	m_freem(m0);

	if (wi_cmd(sc, WI_CMD_TX|WI_RECLAIM, id, 0, 0))
		printf("%s: xmit failed\n", sc->sc_dev.dv_xname);

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;

	return;
}

int
wi_mgmt_xmit(sc, data, len)
	struct wi_softc		*sc;
	caddr_t			data;
	int			len;
{
	struct wi_frame		*tx_frame;
	int			id;
	struct wi_80211_hdr	*hdr;
	caddr_t			dptr;
	struct mbuf		*m0, *m1;

#if defined(OPTIMIZE_RW_DATA)
	wi_rewind(sc);
#endif

	if (!sc->sc_attached)
		return(ENODEV);

	KASSERT(MHLEN >= sizeof(*tx_frame));

	MGETHDR(m0, M_DONTWAIT, MT_DATA);
	if (m0 == NULL)
		return (ENOMEM);
	MGET(m1, M_DONTWAIT, MT_DATA);
	if (m1 == NULL) {
		m_free(m0);
		return (ENOMEM);
	}

	tx_frame = mtod(m0, struct wi_frame *);
	memset(tx_frame, 0, sizeof(*tx_frame));

	hdr = (struct wi_80211_hdr *)data;
	dptr = data + sizeof(struct wi_80211_hdr);

	id = sc->wi_tx_mgmt_id;

	memcpy(&tx_frame->wi_frame_ctl, hdr, sizeof(struct wi_80211_hdr));

	tx_frame->wi_tx_ctl |= htole16(WI_ENC_TX_802_11);
	tx_frame->wi_dat_len = len - sizeof(struct wi_80211_hdr);
	tx_frame->wi_len = htole16(tx_frame->wi_dat_len);

	tx_frame->wi_dat_len = htole16(tx_frame->wi_dat_len);

	wi_write_data(sc, id, 0, (caddr_t)tx_frame, sizeof(*tx_frame));
	wi_write_data(sc, id, sizeof(*tx_frame), dptr,
	    (len - sizeof(struct wi_80211_hdr)));

	m0->m_len = WI_SHORT_802_11_END; /* XXX */
	m0->m_next = m1;
	m1->m_data = dptr;	/* XXX */
	m1->m_len = len - sizeof(struct wi_80211_hdr);
	m1->m_next = 0;

	m0->m_pkthdr.len = m0->m_len + m1->m_len;

	wi_tap_802_11_plus(sc, m0);

	m_freem(m0);

	if (wi_cmd(sc, WI_CMD_TX|WI_RECLAIM, id, 0, 0)) {
		printf("%s: xmit failed\n", sc->sc_dev.dv_xname);
		return(EIO);
	}

	return(0);
}

static void
wi_stop(ifp, disable)
	struct ifnet *ifp;
{
	struct wi_softc	*sc = ifp->if_softc;

	wihap_shutdown(sc);

	if (!sc->sc_attached)
		return;

	CSR_WRITE_2(sc, WI_INT_EN, 0);
	wi_cmd(sc, WI_CMD_DISABLE|sc->wi_portnum, 0, 0, 0);

	callout_stop(&sc->wi_inquire_ch);
	callout_stop(&sc->wi_scan_sh);

	if (disable) {
		if (sc->sc_enabled) {
			if (sc->sc_disable)
				(*sc->sc_disable)(sc);
			sc->sc_enabled = 0;
		}
	}

	ifp->if_flags &= ~(IFF_OACTIVE | IFF_RUNNING);
	ifp->if_timer = 0;
}

static void
wi_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct wi_softc		*sc;

	sc = ifp->if_softc;

	printf("%s: device timeout\n", sc->sc_dev.dv_xname);

	wi_init(ifp);

	ifp->if_oerrors++;

	return;
}

void
wi_shutdown(sc)
	struct wi_softc *sc;
{
	int s;

	s = splnet();
	if (sc->sc_enabled) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}
	wi_stop(&sc->sc_ethercom.ec_if, 0);

	splx(s);
}

int
wi_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct wi_softc *sc = (struct wi_softc *)self;
	int rv = 0, s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ethercom.ec_if);
		break;
	}
	splx(s);
	return (rv);
}

static void
wi_get_id(sc)
	struct wi_softc *sc;
{
	struct wi_ltv_ver       ver;
	struct wi_card_ident	*id;

	/* getting chip identity */
	memset(&ver, 0, sizeof(ver));
	ver.wi_type = WI_RID_CARD_ID;
	ver.wi_len = 5;
	wi_read_record(sc, (struct wi_ltv_gen *)&ver);
	printf("%s: using ", sc->sc_dev.dv_xname);

	sc->sc_firmware_type = WI_NOTYPE;
	for (id = wi_card_ident; id->card_name != NULL; id++) {
		if (le16toh(ver.wi_ver[0]) == id->card_id) {
			printf("%s", id->card_name);
			sc->sc_firmware_type = id->firm_type;
			break;
		}
	}
	if (sc->sc_firmware_type == WI_NOTYPE) {
		if (le16toh(ver.wi_ver[0]) & 0x8000) {
			printf("Unknown PRISM2 chip");
			sc->sc_firmware_type = WI_INTERSIL;
		} else {
			printf("Unknown Lucent chip");
			sc->sc_firmware_type = WI_LUCENT;
		}
	}

	/* get primary firmware version (Only Prism chips) */
	if (sc->sc_firmware_type != WI_LUCENT) {
		memset(&ver, 0, sizeof(ver));
		ver.wi_type = WI_RID_PRI_IDENTITY;
		ver.wi_len = 5;
		wi_read_record(sc, (struct wi_ltv_gen *)&ver);
		LE16TOH(ver.wi_ver[1]);
		LE16TOH(ver.wi_ver[2]);
		LE16TOH(ver.wi_ver[3]);
		sc->sc_pri_firmware_ver = ver.wi_ver[2] * 10000 +
		    ver.wi_ver[3] * 100 + ver.wi_ver[1];
	}

	/* get station firmware version */
	memset(&ver, 0, sizeof(ver));
	ver.wi_type = WI_RID_STA_IDENTITY;
	ver.wi_len = 5;
	wi_read_record(sc, (struct wi_ltv_gen *)&ver);
	LE16TOH(ver.wi_ver[1]);
	LE16TOH(ver.wi_ver[2]);
	LE16TOH(ver.wi_ver[3]);
	sc->sc_sta_firmware_ver = ver.wi_ver[2] * 10000 +
	    ver.wi_ver[3] * 100 + ver.wi_ver[1];
	if (sc->sc_firmware_type == WI_INTERSIL &&
	    (sc->sc_sta_firmware_ver == 10102 || sc->sc_sta_firmware_ver == 20102)) {
		struct wi_ltv_str sver;
		char *p;

		memset(&sver, 0, sizeof(sver));
		sver.wi_type = WI_RID_SYMBOL_IDENTITY;
		sver.wi_len = 7;
		/* value should be the format like "V2.00-11" */
		if (wi_read_record(sc, (struct wi_ltv_gen *)&sver) == 0 &&
		    *(p = (char *)sver.wi_str) >= 'A' &&
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
	    printf("Primary (%u.%u.%u), ", sc->sc_pri_firmware_ver / 10000,
		    (sc->sc_pri_firmware_ver % 10000) / 100,
		    sc->sc_pri_firmware_ver % 100);
	printf("Station (%u.%u.%u)\n",
	    sc->sc_sta_firmware_ver / 10000, (sc->sc_sta_firmware_ver % 10000) / 100,
	    sc->sc_sta_firmware_ver % 100);

	return;
}

int
wi_detach(sc)
	struct wi_softc *sc;
{
	struct ifnet *ifp = sc->sc_ifp;
	int s;

	if (!sc->sc_attached)
		return (0);

	s = splnet();
	callout_stop(&sc->wi_inquire_ch);

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->sc_media, IFM_INST_ANY);

	/* TBD detach all BPF listeners. */
	ether_ifdetach(ifp);
	if_detach(ifp);
	if (sc->sc_enabled) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_enabled = 0;
	}
	splx(s);
	return (0);
}

void
wi_power(sc, why)
	struct wi_softc *sc;
	int why;
{
	int s;

	if (!sc->sc_enabled)
		return;

	s = splnet();
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		wi_stop(sc->sc_ifp, 0);
		if (sc->sc_enabled) {
			if (sc->sc_disable)
				(*sc->sc_disable)(sc);
		}
		break;
	case PWR_RESUME:
		sc->sc_enabled = 0;
		wi_init(sc->sc_ifp);
		(void)wi_intr(sc);
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}

static int
wi_set_ssid(ws, id, len)
	struct ieee80211_nwid *ws;
	u_int8_t *id;
	int len;
{

	if (len > IEEE80211_NWID_LEN)
		return (EINVAL);
	ws->i_len = len;
	memcpy(ws->i_nwid, id, len);
	return (0);
}

static void
wi_request_fill_ssid(wreq, ws)
	struct wi_req *wreq;
	struct ieee80211_nwid *ws;
{
	int len = ws->i_len;

	memset(&wreq->wi_val[0], 0, sizeof(wreq->wi_val));
	wreq->wi_val[0] = htole16(len);
	wreq->wi_len = roundup(len, 2) / 2 + 2;
	memcpy(&wreq->wi_val[1], ws->i_nwid, len);
}

static int
wi_write_ssid(sc, type, wreq, ws)
	struct wi_softc *sc;
	int type;
	struct wi_req *wreq;
	struct ieee80211_nwid *ws;
{

	wreq->wi_type = type;
	wi_request_fill_ssid(wreq, ws);
	return (wi_write_record(sc, (struct wi_ltv_gen *)wreq));
}

static int
wi_sync_media(sc, ptype, txrate)
	struct wi_softc *sc;
	int ptype;
	int txrate;
{
	int media = sc->sc_media.ifm_cur->ifm_media;
	int options = IFM_OPTIONS(media);
	int subtype;

	switch (txrate) {
	case 1:
		subtype = IFM_IEEE80211_DS1;
		break;
	case 2:
		subtype = IFM_IEEE80211_DS2;
		break;
	case 3:
		subtype = IFM_AUTO;
		break;
	case 5:
		subtype = IFM_IEEE80211_DS5;
		break;
	case 11:
		subtype = IFM_IEEE80211_DS11;
		break;
	default:
		subtype = IFM_MANUAL;		/* Unable to represent */
		break;
	}

	options &= ~IFM_OMASK;
	switch (ptype) {
	case WI_PORTTYPE_BSS:
		/* default port type */
		break;
	case WI_PORTTYPE_ADHOC:
		options |= IFM_IEEE80211_ADHOC | IFM_FLAG0;
		break;
	case WI_PORTTYPE_HOSTAP:
		options |= IFM_IEEE80211_HOSTAP;
		break;
	case WI_PORTTYPE_IBSS:
		options |= IFM_IEEE80211_ADHOC;
		break;
	default:
		subtype = IFM_MANUAL;		/* Unable to represent */
		break;
	}
	media = IFM_MAKEWORD(IFM_TYPE(media), subtype, options,
	    IFM_INST(media));
	if (ifmedia_match(&sc->sc_media, media, sc->sc_media.ifm_mask) == NULL)
		return (EINVAL);
	ifmedia_set(&sc->sc_media, media);
	sc->wi_ptype = ptype;
	sc->wi_tx_rate = txrate;
	return (0);
}

static int
wi_media_change(ifp)
	struct ifnet *ifp;
{
	struct wi_softc *sc = ifp->if_softc;
	int otype = sc->wi_ptype;
	int orate = sc->wi_tx_rate;
	int ocreate_ibss = sc->wi_create_ibss;

	sc->wi_create_ibss = 0;

	switch (sc->sc_media.ifm_cur->ifm_media & IFM_OMASK) {
	case 0:
		sc->wi_ptype = WI_PORTTYPE_BSS;
		break;
	case IFM_IEEE80211_HOSTAP:
		sc->wi_ptype = WI_PORTTYPE_HOSTAP;
		break;
	case IFM_IEEE80211_ADHOC:
		sc->wi_ptype = WI_PORTTYPE_IBSS;
		if (sc->wi_flags & WI_FLAGS_HAS_CREATE_IBSS)
			sc->wi_create_ibss = 1;
		break;
	case IFM_IEEE80211_ADHOC | IFM_FLAG0:
		sc->wi_ptype = WI_PORTTYPE_ADHOC;
		break;
	default:
		/* Invalid combination. */
		sc->wi_create_ibss = ocreate_ibss;
		return (EINVAL);
	}

	switch (IFM_SUBTYPE(sc->sc_media.ifm_cur->ifm_media)) {
	case IFM_IEEE80211_DS1:
		sc->wi_tx_rate = 1;
		break;
	case IFM_IEEE80211_DS2:
		sc->wi_tx_rate = 2;
		break;
	case IFM_AUTO:
		sc->wi_tx_rate = 3;
		break;
	case IFM_IEEE80211_DS5:
		sc->wi_tx_rate = 5;
		break;
	case IFM_IEEE80211_DS11:
		sc->wi_tx_rate = 11;
		break;
	}

	if (sc->sc_enabled != 0) {
		if (otype != sc->wi_ptype ||
		    orate != sc->wi_tx_rate ||
		    ocreate_ibss != sc->wi_create_ibss)
			wi_init(ifp);
	}

	ifp->if_baudrate = ifmedia_baudrate(sc->sc_media.ifm_cur->ifm_media);

	return (0);
}

static void
wi_media_status(ifp, imr)
	struct ifnet *ifp;
	struct ifmediareq *imr;
{
	struct wi_req	wreq;
	struct wi_softc *sc = ifp->if_softc;

	if (sc->sc_enabled == 0) {
		imr->ifm_active = IFM_IEEE80211|IFM_NONE;
		imr->ifm_status = 0;
		return;
	}

	if (sc->wi_tx_rate == 3) {
		imr->ifm_active = IFM_IEEE80211|IFM_AUTO;
		switch (sc->wi_ptype) {
		case WI_PORTTYPE_ADHOC:
			imr->ifm_active |= IFM_FLAG0;
			/* fall through */
		case WI_PORTTYPE_IBSS:
			imr->ifm_active |= IFM_IEEE80211_ADHOC;
			break;
		case WI_PORTTYPE_HOSTAP:
			imr->ifm_active |= IFM_IEEE80211_HOSTAP;
			break;
		default:
			/* do nothing for BSS, WDS, or unknown. */
			break;
		}
		wreq.wi_type = WI_RID_CUR_TX_RATE;
		wreq.wi_len = WI_MAX_DATALEN;
		if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq) == 0) {
			switch(wreq.wi_val[0]) {
			case 1:
				imr->ifm_active |= IFM_IEEE80211_DS1;
				break;
			case 2:
				imr->ifm_active |= IFM_IEEE80211_DS2;
				break;
			case 6:
				imr->ifm_active |= IFM_IEEE80211_DS5;
				break;
			case 11:
				imr->ifm_active |= IFM_IEEE80211_DS11;
				break;
			}
		}
	} else {
		imr->ifm_active = sc->sc_media.ifm_cur->ifm_media;
	}

	imr->ifm_status = IFM_AVALID;
	switch (sc->wi_ptype) {
	case WI_PORTTYPE_IBSS:
		/*
		 * XXX: It would be nice if we could give some actually
		 * useful status like whether we joined another IBSS or
		 * created one ourselves.
		 */
		/* fall through */
	case WI_PORTTYPE_HOSTAP:
		imr->ifm_status |= IFM_ACTIVE;
		break;
	case WI_PORTTYPE_BSS:
	default:
#if 0
		wreq.wi_type = WI_RID_COMMQUAL;
		wreq.wi_len = WI_MAX_DATALEN;
		if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq) == 0 &&
		    wreq.wi_val[0] != 0)
#else
		if (sc->wi_flags & WI_FLAGS_CONNECTED)
#endif
			imr->ifm_status |= IFM_ACTIVE;
	}
}

static int
wi_set_nwkey(sc, nwkey)
	struct wi_softc *sc;
	struct ieee80211_nwkey *nwkey;
{
	int i, error;
	size_t len;
	struct wi_req wreq;
	struct wi_ltv_keys *wk = (struct wi_ltv_keys *)&wreq;

	if ((sc->wi_flags & WI_FLAGS_HAS_WEP) == 0)
		return ENODEV;
	if (nwkey->i_defkid <= 0 ||
	    nwkey->i_defkid > IEEE80211_WEP_NKID)
		return EINVAL;
	memcpy(wk, &sc->wi_keys, sizeof(*wk));
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		if (nwkey->i_key[i].i_keydat == NULL)
			continue;
		len = nwkey->i_key[i].i_keylen;
		if (len > sizeof(wk->wi_keys[i].wi_keydat))
			return EINVAL;
		error = copyin(nwkey->i_key[i].i_keydat,
		    wk->wi_keys[i].wi_keydat, len);
		if (error)
			return error;
		wk->wi_keys[i].wi_keylen = htole16(len);
	}

	wk->wi_len = (sizeof(*wk) / 2) + 1;
	wk->wi_type = WI_RID_DEFLT_CRYPT_KEYS;
	if (sc->sc_enabled != 0) {
		error = wi_write_record(sc, (struct wi_ltv_gen *)&wreq);
		if (error)
			return error;
	}
	error = wi_setdef(sc, &wreq);
	if (error)
		return error;

	wreq.wi_len = 2;
	wreq.wi_type = WI_RID_TX_CRYPT_KEY;
	wreq.wi_val[0] = htole16(nwkey->i_defkid - 1);
	if (sc->sc_enabled != 0) {
		error = wi_write_record(sc, (struct wi_ltv_gen *)&wreq);
		if (error)
			return error;
	}
	error = wi_setdef(sc, &wreq);
	if (error)
		return error;

	wreq.wi_type = WI_RID_ENCRYPTION;
	wreq.wi_val[0] = htole16(nwkey->i_wepon);
	if (sc->sc_enabled != 0) {
		error = wi_write_record(sc, (struct wi_ltv_gen *)&wreq);
		if (error)
			return error;
	}
	error = wi_setdef(sc, &wreq);
	if (error)
		return error;

	if (sc->sc_enabled != 0)
		wi_init(&sc->sc_ethercom.ec_if);
	return 0;
}

static int
wi_get_nwkey(sc, nwkey)
	struct wi_softc *sc;
	struct ieee80211_nwkey *nwkey;
{
	int i, len, error;
	struct wi_ltv_keys *wk = &sc->wi_keys;

	if ((sc->wi_flags & WI_FLAGS_HAS_WEP) == 0)
		return ENODEV;
	nwkey->i_wepon = sc->wi_use_wep;
	nwkey->i_defkid = sc->wi_tx_key + 1;

	/* do not show any keys to non-root user */
	error = suser(curproc->p_ucred, &curproc->p_acflag);
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		if (nwkey->i_key[i].i_keydat == NULL)
			continue;
		/* error holds results of suser() for the first time */
		if (error)
			return error;
		len = le16toh(wk->wi_keys[i].wi_keylen);
		if (nwkey->i_key[i].i_keylen < len)
			return ENOSPC;
		nwkey->i_key[i].i_keylen = len;
		error = copyout(wk->wi_keys[i].wi_keydat,
		    nwkey->i_key[i].i_keydat, len);
		if (error)
			return error;
	}
	return 0;
}

static int
wi_set_pm(struct wi_softc *sc, struct ieee80211_power *power)
{

	sc->wi_pm_enabled = power->i_enabled;
	sc->wi_max_sleep = power->i_maxsleep;

	if (sc->sc_enabled)
		return (wi_init(&sc->sc_ethercom.ec_if));

	return (0);
}

static int
wi_get_pm(struct wi_softc *sc, struct ieee80211_power *power)
{

	power->i_enabled = sc->wi_pm_enabled;
	power->i_maxsleep = sc->wi_max_sleep;

	return (0);
}

static int
wi_set_channel(struct wi_softc *sc, struct ieee80211_channel *channel)
{
	int do_init = 0, error = 0;
	if (channel->i_channel != 0 &&
	    !(sc->wi_channels & (1 << (channel->i_channel - 1)))) {
		return EINVAL;
	}
	switch (sc->wi_ptype) {
	default:
	case WI_PORTTYPE_IBSS:
	case WI_PORTTYPE_ADHOC: /* set channel of BSS to join/create */

		if (channel->i_channel != 0 &&
		    channel->i_channel != sc->wi_create_channel) {
			sc->wi_create_channel = channel->i_channel;
			if (sc->sc_enabled)
				do_init = 1;
		}
		/* fall through */
	case WI_PORTTYPE_BSS: /* set channel of BSS to join */

		/* We are warned not to join while a join is pending.
		 * Our next opportunity comes after a LinkStatus notification.
		 */
		if (sc->wi_flags & WI_FLAGS_JOINING) {
			error = EAGAIN;
			break;
		}
		sc->wi_join_channel = channel->i_channel;
		error = wi_join_bss(sc);
		break;
	case WI_PORTTYPE_HOSTAP: /* set channel of BSS to create */
		if (channel->i_channel == 0) {
			error = EINVAL;
			break;
		}
		if (channel->i_channel == sc->wi_create_channel)
			break;
		sc->wi_create_channel = channel->i_channel;
		if (sc->sc_enabled)
			do_init = 1;
		break;
	}
	if (!error && do_init)
		error = wi_init(sc->sc_ifp);
	return error;
}

static int
wi_get_channel(struct wi_softc *sc, struct ieee80211_channel *channel)
{
	switch (sc->wi_ptype) {
	default:
	case WI_PORTTYPE_IBSS:
	case WI_PORTTYPE_ADHOC:
		if (!(sc->wi_flags & WI_FLAGS_CONNECTED)) {
			/* return channel of BSS to create */
			channel->i_channel = sc->wi_create_channel;
			break;
		}
		/* fall through */
	case WI_PORTTYPE_BSS:
		if (sc->wi_flags & WI_FLAGS_CONNECTED) {
			/* return channel of connected BSS */
			channel->i_channel = sc->wi_current_channel;
			break;
		}
		/* return channel of BSS to join */
		channel->i_channel = sc->wi_join_channel;
		break;
	case WI_PORTTYPE_HOSTAP:
		/* return channel of BSS to create */
		channel->i_channel = sc->wi_create_channel;
		break;
	}
	return 0;
}
