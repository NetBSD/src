/*	$NetBSD: wi.c,v 1.16 2001/06/04 03:34:47 toshii Exp $	*/

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

#define WI_HERMES_AUTOINC_WAR	/* Work around data write autoinc bug. */
#define WI_HERMES_STATS_WAR	/* Work around stats counter bug. */

#include "opt_inet.h"
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
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_ieee80211.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

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

static int wi_cmd		__P((struct wi_softc *, int, int));
static int wi_read_record	__P((struct wi_softc *, struct wi_ltv_gen *));
static int wi_write_record	__P((struct wi_softc *, struct wi_ltv_gen *));
static int wi_read_data		__P((struct wi_softc *, int,
					int, caddr_t, int));
static int wi_write_data	__P((struct wi_softc *, int,
					int, caddr_t, int));
static int wi_seek		__P((struct wi_softc *, int, int, int));
static int wi_alloc_nicmem	__P((struct wi_softc *, int, int *));
static void wi_inquire		__P((void *));
static int wi_setdef		__P((struct wi_softc *, struct wi_req *));
static int wi_getdef		__P((struct wi_softc *, struct wi_req *));
static int wi_mgmt_xmit		__P((struct wi_softc *, caddr_t, int));

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
static int wi_set_pm(struct wi_softc *, struct ieee80211_power *);
static int wi_get_pm(struct wi_softc *, struct ieee80211_power *);

int
wi_attach(sc)
	struct wi_softc *sc;
{
	struct ifnet *ifp = sc->sc_ifp;
	struct wi_ltv_macaddr   mac;
	struct wi_ltv_gen       gen;
	static const u_int8_t empty_macaddr[ETHER_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int s;

	s = splnet();

	callout_init(&sc->wi_inquire_ch);

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
	if (bcmp(sc->sc_macaddr, empty_macaddr, ETHER_ADDR_LEN) == 0) {
		printf("%s: could not get mac address, attach failed\n",
		    sc->sc_dev.dv_xname);
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
	sc->wi_channel = le16toh(gen.wi_val);

	bzero((char *)&sc->wi_stats, sizeof(sc->wi_stats));

	/*
	 * Find out if we support WEP on this card.
	 */
	gen.wi_type = WI_RID_WEP_AVAIL;
	gen.wi_len = 2;
	wi_read_record(sc, &gen);
	sc->wi_has_wep = le16toh(gen.wi_val);

	ifmedia_init(&sc->sc_media, 0, wi_media_change, wi_media_status);
#define	IFM_AUTOADHOC \
	IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO, IFM_IEEE80211_ADHOC, 0)
#define	ADD(m, c)	ifmedia_add(&sc->sc_media, (m), (c), NULL)
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_AUTO, 0, 0), 0);
	ADD(IFM_AUTOADHOC, 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS1, 0, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS1,
	    IFM_IEEE80211_ADHOC, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS2, 0, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS2,
	    IFM_IEEE80211_ADHOC, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS11, 0, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_IEEE80211_DS11,
	    IFM_IEEE80211_ADHOC, 0), 0);
	ADD(IFM_MAKEWORD(IFM_IEEE80211, IFM_MANUAL, 0, 0), 0);
#undef ADD
	ifmedia_set(&sc->sc_media, IFM_AUTOADHOC);

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, mac.wi_mac_addr);

	ifp->if_baudrate = IF_Mbps(2);

	/* Attach is successful. */
	sc->sc_attached = 1;

	splx(s);
	return 0;
}

static void wi_rxeof(sc)
	struct wi_softc		*sc;
{
	struct ifnet		*ifp;
	struct ether_header	*eh;
	struct wi_frame		rx_frame;
	struct mbuf		*m;
	int			id;

	ifp = sc->sc_ifp;

	id = CSR_READ_2(sc, WI_RX_FID);

	/* First read in the frame header */
	if (wi_read_data(sc, id, 0, (caddr_t)&rx_frame, sizeof(rx_frame))) {
		ifp->if_ierrors++;
		return;
	}

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

	/* Align the data after the ethernet header */
	m->m_data = (caddr_t) ALIGN(m->m_data + sizeof(struct ether_header))
	    - sizeof(struct ether_header);

	eh = mtod(m, struct ether_header *);
	m->m_pkthdr.rcvif = ifp;

	if (le16toh(rx_frame.wi_status) == WI_STAT_1042 ||
	    le16toh(rx_frame.wi_status) == WI_STAT_TUNNEL ||
	    le16toh(rx_frame.wi_status) == WI_STAT_WMP_MSG) {
		if ((le16toh(rx_frame.wi_dat_len) + WI_SNAPHDR_LEN) > MCLBYTES) {
			printf("%s: oversized packet received "
			    "(wi_dat_len=%d, wi_status=0x%x)\n",
			    sc->sc_dev.dv_xname,
			    le16toh(rx_frame.wi_dat_len), le16toh(rx_frame.wi_status));
			m_freem(m);
			ifp->if_ierrors++;
			return;
		}
		m->m_pkthdr.len = m->m_len =
		    le16toh(rx_frame.wi_dat_len) + WI_SNAPHDR_LEN;

		bcopy((char *)&rx_frame.wi_dst_addr,
		    (char *)&eh->ether_dhost, ETHER_ADDR_LEN);
		bcopy((char *)&rx_frame.wi_src_addr,
		    (char *)&eh->ether_shost, ETHER_ADDR_LEN);
		bcopy((char *)&rx_frame.wi_type,
		    (char *)&eh->ether_type, sizeof(u_int16_t));

		if (wi_read_data(sc, id, WI_802_11_OFFSET,
		    mtod(m, caddr_t) + sizeof(struct ether_header),
		    m->m_len + 2)) {
			m_freem(m);
			ifp->if_ierrors++;
			return;
		}
	} else {
		if ((le16toh(rx_frame.wi_dat_len) +
		    sizeof(struct ether_header)) > MCLBYTES) {
			printf("%s: oversized packet received "
			    "(wi_dat_len=%d, wi_status=0x%x)\n",
			    sc->sc_dev.dv_xname,
			    le16toh(rx_frame.wi_dat_len), le16toh(rx_frame.wi_status));
			m_freem(m);
			ifp->if_ierrors++;
			return;
		}
		m->m_pkthdr.len = m->m_len =
		    le16toh(rx_frame.wi_dat_len) + sizeof(struct ether_header);

		if (wi_read_data(sc, id, WI_802_3_OFFSET,
		    mtod(m, caddr_t), m->m_len + 2)) {
			m_freem(m);
			ifp->if_ierrors++;
			return;
		}
	}

	ifp->if_ipackets++;

#if NBPFILTER > 0
	/* Handle BPF listeners. */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m);
#endif

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

	sc = xsc;
	ifp = &sc->sc_ethercom.ec_if;

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return;

	callout_reset(&sc->wi_inquire_ch, hz * 60, wi_inquire, sc);

	/* Don't do this while we're transmitting */
	if (ifp->if_flags & IFF_OACTIVE)
		return;

	wi_cmd(sc, WI_CMD_INQUIRE, WI_INFO_COUNTERS);

	return;
}

void wi_update_stats(sc)
	struct wi_softc		*sc;
{
	struct wi_ltv_gen	gen;
	u_int16_t		id;
	struct ifnet		*ifp;
	u_int32_t		*ptr;
	int			len, i;
	u_int16_t		t;

	ifp = &sc->sc_ethercom.ec_if;

	id = CSR_READ_2(sc, WI_INFO_FID);

	wi_read_data(sc, id, 0, (char *)&gen, 4);

	if (gen.wi_type != WI_INFO_COUNTERS)
		return;

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

	return;
}

int wi_intr(arg)
	void *arg;
{
	struct wi_softc		*sc = arg;
	struct ifnet		*ifp;
	u_int16_t		status;

	if (sc->sc_enabled == 0 ||
	    (sc->sc_dev.dv_flags & DVF_ACTIVE) == 0 ||
	    (sc->sc_ethercom.ec_if.if_flags & IFF_RUNNING) == 0)
		return (0);

	ifp = &sc->sc_ethercom.ec_if;

	if (!(ifp->if_flags & IFF_UP)) {
		CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);
		CSR_WRITE_2(sc, WI_INT_EN, 0);
		return 1;
	}

	/* Disable interrupts. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);

	status = CSR_READ_2(sc, WI_EVENT_STAT);
	CSR_WRITE_2(sc, WI_EVENT_ACK, ~WI_INTRS);

	if (status & WI_EV_RX) {
		wi_rxeof(sc);
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_RX);
	}

	if (status & WI_EV_TX) {
		wi_txeof(sc, status);
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_TX);
	}

	if (status & WI_EV_ALLOC) {
		int			id;
		id = CSR_READ_2(sc, WI_ALLOC_FID);
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_ALLOC);
		if (id == sc->wi_tx_data_id)
			wi_txeof(sc, status);
	}

	if (status & WI_EV_INFO) {
		wi_update_stats(sc);
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_INFO);
	}

	if (status & WI_EV_TX_EXC) {
		wi_txeof(sc, status);
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_TX_EXC);
	}

	if (status & WI_EV_INFO_DROP) {
		CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_INFO_DROP);
	}

	/* Re-enable interrupts. */
	CSR_WRITE_2(sc, WI_INT_EN, WI_INTRS);

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		wi_start(ifp);

	return 1;
}

static int
wi_cmd(sc, cmd, val)
	struct wi_softc		*sc;
	int			cmd;
	int			val;
{
	int			i, s = 0;

	/* wait for the busy bit to clear */
	for (i = 0; i < WI_TIMEOUT; i++) {
		if (!(CSR_READ_2(sc, WI_COMMAND) & WI_CMD_BUSY))
			break;
	}

	CSR_WRITE_2(sc, WI_PARAM0, val);
	CSR_WRITE_2(sc, WI_PARAM1, 0);
	CSR_WRITE_2(sc, WI_PARAM2, 0);
	CSR_WRITE_2(sc, WI_COMMAND, cmd);

	/* wait for the cmd completed bit */
	for (i = 0; i < WI_TIMEOUT; i++) {
		if (CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_CMD)
			break;
		DELAY(1);
	}

	/* Ack the command */
	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_CMD);

	s = CSR_READ_2(sc, WI_STATUS);
	if (s & WI_STAT_CMD_RESULT)
		return(EIO);

	if (i == WI_TIMEOUT)
		return(ETIMEDOUT);

	return(0);
}

static void
wi_reset(sc)
	struct wi_softc		*sc;
{
	DELAY(100*1000); /* 100 m sec */
	if (wi_cmd(sc, WI_CMD_INI, 0))
		printf("%s: init failed\n", sc->sc_dev.dv_xname);
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

	if (sc->sc_prism2) {
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
	if (wi_cmd(sc, WI_CMD_ACCESS|WI_ACCESS_READ, ltv->wi_type))
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

	/* Now read the data. */
	ptr = &ltv->wi_val;
	if (ltv->wi_len > 1)
		CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr, ltv->wi_len - 1);

	if (sc->sc_prism2) {
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
		case WI_RID_AUTH_CNTL:
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

	if (sc->sc_prism2) {
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
			if (le16toh(ltv->wi_val))
				p2ltv.wi_val = htole16(0x03);
			else
				p2ltv.wi_val = htole16(0x90);
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
			struct wi_ltv_str	ws;
			struct wi_ltv_keys	*wk = (struct wi_ltv_keys *)ltv;
			for (i = 0; i < 4; i++) {
				ws.wi_len = 4;
				ws.wi_type = WI_RID_P2_CRYPT_KEY0 + i;
				memcpy(ws.wi_str, &wk->wi_keys[i].wi_keydat, 5);
				ws.wi_str[5] = '\0';
				error = wi_write_record(sc,
				    (struct wi_ltv_gen *)&ws);
				if (error)
					return error;
			}
			return 0;
		    }
		case WI_RID_AUTH_CNTL:
			p2ltv.wi_type = WI_RID_AUTH_CNTL;
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

	if (wi_cmd(sc, WI_CMD_ACCESS|WI_ACCESS_WRITE, ltv->wi_type))
		return(EIO);

	return(0);
}

static int wi_seek(sc, id, off, chan)
	struct wi_softc		*sc;
	int			id, off, chan;
{
	int			i;
	int			selreg, offreg;
	int 			status;

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

	for (i = 0; i < WI_TIMEOUT; i++) {
	  	status = CSR_READ_2(sc, offreg);
		if (!(status & (WI_OFF_BUSY|WI_OFF_ERR)))
			break;
	}

	if (i == WI_TIMEOUT) {
		printf("%s: timeout in wi_seek to %x/%x; last status %x\n",
		       sc->sc_dev.dv_xname, id, off, status);
		return(ETIMEDOUT);
	}
	return(0);
}

static int wi_read_data(sc, id, off, buf, len)
	struct wi_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	u_int16_t		*ptr;

	if (wi_seek(sc, id, off, WI_BAP1))
		return(EIO);

	ptr = (u_int16_t *)buf;
	CSR_READ_MULTI_STREAM_2(sc, WI_DATA1, ptr, len / 2);

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
 */
static int wi_write_data(sc, id, off, buf, len)
	struct wi_softc		*sc;
	int			id, off;
	caddr_t			buf;
	int			len;
{
	u_int16_t		*ptr;

#ifdef WI_HERMES_AUTOINC_WAR
again:
#endif

	if (wi_seek(sc, id, off, WI_BAP0))
		return(EIO);

	ptr = (u_int16_t *)buf;
	CSR_WRITE_MULTI_STREAM_2(sc, WI_DATA0, ptr, len / 2);

#ifdef WI_HERMES_AUTOINC_WAR
	CSR_WRITE_2(sc, WI_DATA0, 0x1234);
	CSR_WRITE_2(sc, WI_DATA0, 0x5678);

	if (wi_seek(sc, id, off + len, WI_BAP0))
		return(EIO);

	if (CSR_READ_2(sc, WI_DATA0) != 0x1234 ||
	    CSR_READ_2(sc, WI_DATA0) != 0x5678)
		goto again;
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

	if (wi_cmd(sc, WI_CMD_ALLOC_MEM, len)) {
		printf("%s: failed to allocate %d bytes on NIC\n",
		    sc->sc_dev.dv_xname, len);
		return(ENOMEM);
	}

	for (i = 0; i < WI_TIMEOUT; i++) {
		if (CSR_READ_2(sc, WI_EVENT_STAT) & WI_EV_ALLOC)
			break;
	}

	if (i == WI_TIMEOUT) {
		printf("%s: TIMED OUT in alloc\n", sc->sc_dev.dv_xname);
		return(ETIMEDOUT);
	}

	CSR_WRITE_2(sc, WI_EVENT_ACK, WI_EV_ALLOC);
	*id = CSR_READ_2(sc, WI_ALLOC_FID);

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
		bzero((char *)&mcast, sizeof(mcast));
		mcast.wi_type = WI_RID_MCAST;
		mcast.wi_len = ((ETHER_ADDR_LEN / 2) * 16) + 1;

		wi_write_record(sc, (struct wi_ltv_gen *)&mcast);
		return;
	}

	i = 0;
	ETHER_FIRST_MULTI(estep, ec, enm);
	while (enm != NULL) {
		/* Punt on ranges or too many multicast addresses. */
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0 ||
		    i >= 16)
			goto allmulti;

		bcopy(enm->enm_addrlo,
		    (char *)&mcast.wi_mcast[i], ETHER_ADDR_LEN);
		i++;
		ETHER_NEXT_MULTI(estep, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	mcast.wi_type = WI_RID_MCAST;
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
		bcopy((char *)&wreq->wi_val, (char *)&sc->sc_macaddr,
		    ETHER_ADDR_LEN);
		bcopy((char *)&wreq->wi_val, LLADDR(sdl), ETHER_ADDR_LEN);
		break;
	case WI_RID_PORTTYPE:
		error = wi_sync_media(sc, le16toh(wreq->wi_val[0]), sc->wi_tx_rate);
		break;
	case WI_RID_TX_RATE:
		error = wi_sync_media(sc, sc->wi_ptype, le16toh(wreq->wi_val[0]));
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
		break;
	case WI_RID_OWN_CHNL:
		sc->wi_channel = le16toh(wreq->wi_val[0]);
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
	case WI_RID_AUTH_CNTL:
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
		bcopy((char *)wreq, (char *)&sc->wi_keys,
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
		bcopy(&sc->sc_macaddr, &wreq->wi_val, ETHER_ADDR_LEN);
		bcopy(LLADDR(sdl), &wreq->wi_val, ETHER_ADDR_LEN);
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
		wreq->wi_val[0] = htole16(sc->wi_channel);
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
	case WI_RID_AUTH_CNTL:
		wreq->wi_val[0] = htole16(sc->wi_authtype);
		break;
	case WI_RID_ROAMING_MODE:
		wreq->wi_val[0] = htole16(sc->wi_roaming);
		break;
	case WI_RID_WEP_AVAIL:
		wreq->wi_val[0] = htole16(sc->wi_has_wep);
		break;
	case WI_RID_ENCRYPTION:
		wreq->wi_val[0] = htole16(sc->wi_use_wep);
		break;
	case WI_RID_TX_CRYPT_KEY:
		wreq->wi_val[0] = htole16(sc->wi_tx_key);
		break;
	case WI_RID_DEFLT_CRYPT_KEYS:
		wreq->wi_len += sizeof(struct wi_ltv_keys) / 2 - 1;
		bcopy(&sc->wi_keys, wreq, sizeof(struct wi_ltv_keys));
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
	struct wi_softc		*sc = ifp->if_softc;
	struct wi_req		wreq;
	struct ifreq		*ifr;
	struct proc *p = curproc;
	struct ieee80211_nwid nwid;

	if ((sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENXIO);

	s = splnet();

	ifr = (struct ifreq *)data;
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
				WI_SETVAL(WI_RID_PROMISC, 1);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC) &&
			    sc->wi_if_flags & IFF_PROMISC) {
				WI_SETVAL(WI_RID_PROMISC, 0);
			}
			wi_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING) {
				wi_stop(ifp, 0);
			}
		}
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
		if (wreq.wi_type == WI_RID_IFACE_STATS) {
			/* XXX native byte order */
			bcopy((char *)&sc->wi_stats, (char *)&wreq.wi_val,
			    sizeof(sc->wi_stats));
			wreq.wi_len = (sizeof(sc->wi_stats) / 2) + 1;
		} else if (wreq.wi_type == WI_RID_DEFLT_CRYPT_KEYS) {
			/* For non-root user, return all-zeroes keys */
			if (suser(p->p_ucred, &p->p_acflag))
				bzero((char *)&wreq,
				    sizeof(struct wi_ltv_keys));
			else
				bcopy((char *)&sc->wi_keys, (char *)&wreq,
				    sizeof(struct wi_ltv_keys));
		} else {
			if (sc->sc_enabled == 0)
				error = wi_getdef(sc, &wreq);
			else if (wi_read_record(sc, (struct wi_ltv_gen *)&wreq))
				error = EINVAL;
		}
		if (error == 0)
			error = copyout(&wreq, ifr->ifr_data, sizeof(wreq));
		break;
	case SIOCSWAVELAN:
		error = suser(p->p_ucred, &p->p_acflag);
		if (error)
			break;
		error = copyin(ifr->ifr_data, &wreq, sizeof(wreq));
		if (error)
			break;
		if (wreq.wi_type == WI_RID_IFACE_STATS) {
			error = EINVAL;
			break;
		} else if (wreq.wi_type == WI_RID_MGMT_XMIT) {
			error = wi_mgmt_xmit(sc, (caddr_t)&wreq.wi_val,
			    wreq.wi_len);
		} else {
			if (sc->sc_enabled != 0)
				error = wi_write_record(sc,
				    (struct wi_ltv_gen *)&wreq);
			if (error == 0)
				error = wi_setdef(sc, &wreq);
			if (error == 0 && sc->sc_enabled != 0)
				/* Reinitialize WaveLAN. */
				wi_init(ifp);
		}
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
	int error, id = 0;

	if (!sc->sc_enabled) {
		if ((error = (*sc->sc_enable)(sc)) != 0)
			goto out;
		sc->sc_enabled = 1;
	}

	wi_stop(ifp, 0);
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
	WI_SETVAL(WI_RID_ROAMING_MODE, sc->wi_roaming);

	/* Specify the IBSS name */
	wi_write_ssid(sc, WI_RID_OWN_SSID, &wreq, &sc->wi_ibssid);

	/* Specify the network name */
	wi_write_ssid(sc, WI_RID_DESIRED_SSID, &wreq, &sc->wi_netid);

	/* Specify the frequency to use */
	WI_SETVAL(WI_RID_OWN_CHNL, sc->wi_channel);

	/* Program the nodename. */
	wi_write_ssid(sc, WI_RID_NODENAME, &wreq, &sc->wi_nodeid);

	/* Set our MAC address. */
	mac.wi_len = 4;
	mac.wi_type = WI_RID_MAC_NODE;
	memcpy(&mac.wi_mac_addr, sc->sc_macaddr, ETHER_ADDR_LEN);
	wi_write_record(sc, (struct wi_ltv_gen *)&mac);

	/* Initialize promisc mode. */
	if (ifp->if_flags & IFF_PROMISC) {
		WI_SETVAL(WI_RID_PROMISC, 1);
	} else {
		WI_SETVAL(WI_RID_PROMISC, 0);
	}

	/* Configure WEP. */
	if (sc->wi_has_wep) {
		WI_SETVAL(WI_RID_ENCRYPTION, sc->wi_use_wep);
		WI_SETVAL(WI_RID_TX_CRYPT_KEY, sc->wi_tx_key);
		sc->wi_keys.wi_len = (sizeof(struct wi_ltv_keys) / 2) + 1;
		sc->wi_keys.wi_type = WI_RID_DEFLT_CRYPT_KEYS;
		wi_write_record(sc, (struct wi_ltv_gen *)&sc->wi_keys);
		if (sc->sc_prism2 && sc->wi_use_wep) {
			/*
			 * ONLY HWB3163 EVAL-CARD Firmware version
			 * less than 0.8 variant3
			 *
			 *   If promiscuous mode disable, Prism2 chip
			 *  does not work with WEP .
			 * It is under investigation for details.
			 * (ichiro@netbsd.org)
			 */
			if (sc->sc_prism2_ver < 83 ) {
				/* firm ver < 0.8 variant 3 */
				WI_SETVAL(WI_RID_PROMISC, 1);
			}
			WI_SETVAL(WI_RID_AUTH_CNTL, sc->wi_authtype);
		}
	}

	/* Set multicast filter. */
	wi_setmulti(sc);

	/* Enable desired port */
	wi_cmd(sc, WI_CMD_ENABLE | sc->wi_portnum, 0);

	if ((error = wi_alloc_nicmem(sc,
	    1518 + sizeof(struct wi_frame) + 8, &id)) != 0) {
		printf("%s: tx buffer allocation failed\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}
	sc->wi_tx_data_id = id;

	if ((error = wi_alloc_nicmem(sc,
	    1518 + sizeof(struct wi_frame) + 8, &id)) != 0) {
		printf("%s: mgmt. buffer allocation failed\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}
	sc->wi_tx_mgmt_id = id;

	/* Enable interrupts */
	CSR_WRITE_2(sc, WI_INT_EN, WI_INTRS);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_reset(&sc->wi_inquire_ch, hz * 60, wi_inquire, sc);

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	}
	return (error);
}

static void
wi_start(ifp)
	struct ifnet		*ifp;
{
	struct wi_softc		*sc;
	struct mbuf		*m0;
	struct wi_frame		tx_frame;
	struct ether_header	*eh;
	int			id;

	sc = ifp->if_softc;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	IFQ_DEQUEUE(&ifp->if_snd, m0);
	if (m0 == NULL)
		return;

	bzero((char *)&tx_frame, sizeof(tx_frame));
	id = sc->wi_tx_data_id;
	eh = mtod(m0, struct ether_header *);

	/*
	 * Use RFC1042 encoding for IP and ARP datagrams,
	 * 802.3 for anything else.
	 */
	if (ntohs(eh->ether_type) == ETHERTYPE_IP ||
	    ntohs(eh->ether_type) == ETHERTYPE_ARP ||
	    ntohs(eh->ether_type) == ETHERTYPE_REVARP ||
	    ntohs(eh->ether_type) == ETHERTYPE_IPV6) {
		bcopy((char *)&eh->ether_dhost,
		    (char *)&tx_frame.wi_addr1, ETHER_ADDR_LEN);
		bcopy((char *)&eh->ether_shost,
		    (char *)&tx_frame.wi_addr2, ETHER_ADDR_LEN);
		bcopy((char *)&eh->ether_dhost,
		    (char *)&tx_frame.wi_dst_addr, ETHER_ADDR_LEN);
		bcopy((char *)&eh->ether_shost,
		    (char *)&tx_frame.wi_src_addr, ETHER_ADDR_LEN);

		tx_frame.wi_dat_len = htole16(m0->m_pkthdr.len - WI_SNAPHDR_LEN);
		tx_frame.wi_frame_ctl = htole16(WI_FTYPE_DATA);
		tx_frame.wi_dat[0] = htons(WI_SNAP_WORD0);
		tx_frame.wi_dat[1] = htons(WI_SNAP_WORD1);
		tx_frame.wi_len = htons(m0->m_pkthdr.len - WI_SNAPHDR_LEN);
		tx_frame.wi_type = eh->ether_type;

		m_copydata(m0, sizeof(struct ether_header),
		    m0->m_pkthdr.len - sizeof(struct ether_header),
		    (caddr_t)&sc->wi_txbuf);

		wi_write_data(sc, id, 0, (caddr_t)&tx_frame,
		    sizeof(struct wi_frame));
		wi_write_data(sc, id, WI_802_11_OFFSET, (caddr_t)&sc->wi_txbuf,
		    (m0->m_pkthdr.len - sizeof(struct ether_header)) + 2);
	} else {
		tx_frame.wi_dat_len = htole16(m0->m_pkthdr.len);

		m_copydata(m0, 0, m0->m_pkthdr.len, (caddr_t)&sc->wi_txbuf);

		wi_write_data(sc, id, 0, (caddr_t)&tx_frame,
		    sizeof(struct wi_frame));
		wi_write_data(sc, id, WI_802_3_OFFSET, (caddr_t)&sc->wi_txbuf,
		    m0->m_pkthdr.len + 2);
	}

#if NBPFILTER > 0
	/*
	 * If there's a BPF listener, bounce a copy of
	 * this frame to him.
	 */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	m_freem(m0);

	if (wi_cmd(sc, WI_CMD_TX|WI_RECLAIM, id))
		printf("%s: xmit failed\n", sc->sc_dev.dv_xname);

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;

	return;
}

static int
wi_mgmt_xmit(sc, data, len)
	struct wi_softc		*sc;
	caddr_t			data;
	int			len;
{
	struct wi_frame		tx_frame;
	int			id;
	struct wi_80211_hdr	*hdr;
	caddr_t			dptr;

	hdr = (struct wi_80211_hdr *)data;
	dptr = data + sizeof(struct wi_80211_hdr);

	bzero((char *)&tx_frame, sizeof(tx_frame));
	id = sc->wi_tx_mgmt_id;

	bcopy((char *)hdr, (char *)&tx_frame.wi_frame_ctl,
	   sizeof(struct wi_80211_hdr));

	tx_frame.wi_dat_len = htole16(len - WI_SNAPHDR_LEN);
	tx_frame.wi_len = htons(len - WI_SNAPHDR_LEN);

	wi_write_data(sc, id, 0, (caddr_t)&tx_frame, sizeof(struct wi_frame));
	wi_write_data(sc, id, WI_802_11_OFFSET_RAW, dptr,
	    (len - sizeof(struct wi_80211_hdr)) + 2);

	if (wi_cmd(sc, WI_CMD_TX|WI_RECLAIM, id)) {
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

	CSR_WRITE_2(sc, WI_INT_EN, 0);
	wi_cmd(sc, WI_CMD_DISABLE|sc->wi_portnum, 0);

	callout_stop(&sc->wi_inquire_ch);

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

	/* getting chip identity */
	memset(&ver, 0, sizeof(ver));
	ver.wi_type = WI_RID_CARDID;
	ver.wi_len = 5;
	wi_read_record(sc, (struct wi_ltv_gen *)&ver);
	printf("%s: using ", sc->sc_dev.dv_xname);
	switch (le16toh(ver.wi_ver[0])) {
		case WI_NIC_EVB2:
			printf("RF:PRISM2 MAC:HFA3841");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_HWB3763:
			printf("RF:PRISM2 MAC:HFA3841 CARD:HWB3763 rev.B");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_HWB3163:
			printf("RF:PRISM2 MAC:HFA3841 CARD:HWB3163 rev.A");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_HWB3163B:
			printf("RF:PRISM2 MAC:HFA3841 CARD:HWB3163 rev.B");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_EVB3:
			printf("RF:PRISM2 MAC:HFA3842");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_HWB1153:
			printf("RF:PRISM1 MAC:HFA3841 CARD:HWB1153");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_P2_SST:
			printf("RF:PRISM2 MAC:HFA3841 CARD:HWB3163-SST-flash");
			sc->sc_prism2 = 1;
			break;
		case WI_NIC_PRISM2_5:
			printf("RF:PRISM2.5 MAC:ISL3873");
			sc->sc_prism2 = 1;
			break;
		default:
			printf("Lucent chip or unknown chip\n");
			sc->sc_prism2 = 0;
			break;
	}

	if (sc->sc_prism2) {
		/* try to get prism2 firm version */
		memset(&ver, 0, sizeof(ver));
		ver.wi_type = WI_RID_IDENT;
		ver.wi_len = 5;
		wi_read_record(sc, (struct wi_ltv_gen *)&ver);
		LE16TOH(ver.wi_ver[1]);
		LE16TOH(ver.wi_ver[2]);
		LE16TOH(ver.wi_ver[3]);
		printf(" ,Firmware: %i.%i variant %i\n", ver.wi_ver[2],
		       ver.wi_ver[3], ver.wi_ver[1]);
		sc->sc_prism2_ver = ver.wi_ver[2] * 100 +
				    ver.wi_ver[3] *  10 + ver.wi_ver[1];
	}

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
	case 11:
		subtype = IFM_IEEE80211_DS11;
		break;
	default:
		subtype = IFM_MANUAL;		/* Unable to represent */
		break;
	}
	switch (ptype) {
	case WI_PORTTYPE_ADHOC:
		options |= IFM_IEEE80211_ADHOC;
		break;
	case WI_PORTTYPE_BSS:
		options &= ~IFM_IEEE80211_ADHOC;
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

	if ((sc->sc_media.ifm_cur->ifm_media & IFM_IEEE80211_ADHOC) != 0)
		sc->wi_ptype = WI_PORTTYPE_ADHOC;
	else
		sc->wi_ptype = WI_PORTTYPE_BSS;

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
	case IFM_IEEE80211_DS11:
		sc->wi_tx_rate = 11;
		break;
	}

	if (sc->sc_enabled != 0) {
		if (otype != sc->wi_ptype ||
		    orate != sc->wi_tx_rate)
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
	struct wi_softc *sc = ifp->if_softc;

	if (sc->sc_enabled == 0) {
		imr->ifm_active = IFM_IEEE80211|IFM_NONE;
		imr->ifm_status = 0;
		return;
	}

	imr->ifm_active = sc->sc_media.ifm_cur->ifm_media;
	imr->ifm_status = IFM_AVALID|IFM_ACTIVE;
}

static int
wi_set_nwkey(sc, nwkey)
	struct wi_softc *sc;
	struct ieee80211_nwkey *nwkey;
{
	int i, len, error;
	struct wi_req wreq;
	struct wi_ltv_keys *wk = (struct wi_ltv_keys *)&wreq;

	if (!sc->wi_has_wep)
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

	if (!sc->wi_has_wep)
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
