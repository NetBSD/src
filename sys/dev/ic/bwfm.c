/* $NetBSD: bwfm.c,v 1.4.2.2 2017/12/03 11:37:03 jdolecek Exp $ */
/* $OpenBSD: bwfm.c,v 1.5 2017/10/16 22:27:16 patrick Exp $ */
/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/kmem.h>
#include <sys/workqueue.h>
#include <sys/pcq.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>

#include <dev/ic/bwfmvar.h>
#include <dev/ic/bwfmreg.h>

/* #define BWFM_DEBUG */
#ifdef BWFM_DEBUG
#define DPRINTF(x)	do { if (bwfm_debug > 0) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (bwfm_debug >= (n)) printf x; } while (0)
static int bwfm_debug = 1;
#else
#define DPRINTF(x)	do { ; } while (0)
#define DPRINTFN(n, x)	do { ; } while (0)
#endif

#define DEVNAME(sc)	device_xname((sc)->sc_dev)

void	 bwfm_start(struct ifnet *);
int	 bwfm_init(struct ifnet *);
void	 bwfm_stop(struct ifnet *, int);
void	 bwfm_watchdog(struct ifnet *);
int	 bwfm_ioctl(struct ifnet *, u_long, void *);
int	 bwfm_media_change(struct ifnet *);
void	 bwfm_media_status(struct ifnet *, struct ifmediareq *);

int	 bwfm_send_mgmt(struct ieee80211com *, struct ieee80211_node *,
	     int, int);
void	 bwfm_recv_mgmt(struct ieee80211com *, struct mbuf *,
	     struct ieee80211_node *, int, int, uint32_t);
int	 bwfm_key_set(struct ieee80211com *, const struct ieee80211_key *,
	     const uint8_t *);
int	 bwfm_key_delete(struct ieee80211com *, const struct ieee80211_key *);
int	 bwfm_newstate(struct ieee80211com *, enum ieee80211_state, int);
void	 bwfm_newstate_cb(struct bwfm_softc *, struct bwfm_cmd_newstate *);
void	 bwfm_newassoc(struct ieee80211_node *, int);
void	 bwfm_task(struct work *, void *);

int	 bwfm_chip_attach(struct bwfm_softc *);
int	 bwfm_chip_detach(struct bwfm_softc *, int);
struct bwfm_core *bwfm_chip_get_core(struct bwfm_softc *, int);
struct bwfm_core *bwfm_chip_get_pmu(struct bwfm_softc *);
int	 bwfm_chip_ai_isup(struct bwfm_softc *, struct bwfm_core *);
void	 bwfm_chip_ai_disable(struct bwfm_softc *, struct bwfm_core *,
	     uint32_t, uint32_t);
void	 bwfm_chip_ai_reset(struct bwfm_softc *, struct bwfm_core *,
	     uint32_t, uint32_t, uint32_t);
void	 bwfm_chip_dmp_erom_scan(struct bwfm_softc *);
int	 bwfm_chip_dmp_get_regaddr(struct bwfm_softc *, uint32_t *,
	     uint32_t *, uint32_t *);
void	 bwfm_chip_cr4_set_passive(struct bwfm_softc *);
void	 bwfm_chip_ca7_set_passive(struct bwfm_softc *);
void	 bwfm_chip_cm3_set_passive(struct bwfm_softc *);

int	 bwfm_proto_bcdc_query_dcmd(struct bwfm_softc *, int,
	     int, char *, size_t *);
int	 bwfm_proto_bcdc_set_dcmd(struct bwfm_softc *, int,
	     int, char *, size_t);

int	 bwfm_fwvar_cmd_get_data(struct bwfm_softc *, int, void *, size_t);
int	 bwfm_fwvar_cmd_set_data(struct bwfm_softc *, int, void *, size_t);
int	 bwfm_fwvar_cmd_get_int(struct bwfm_softc *, int, uint32_t *);
int	 bwfm_fwvar_cmd_set_int(struct bwfm_softc *, int, uint32_t);
int	 bwfm_fwvar_var_get_data(struct bwfm_softc *, const char *, void *, size_t);
int	 bwfm_fwvar_var_set_data(struct bwfm_softc *, const char *, void *, size_t);
int	 bwfm_fwvar_var_get_int(struct bwfm_softc *, const char *, uint32_t *);
int	 bwfm_fwvar_var_set_int(struct bwfm_softc *, const char *, uint32_t);

struct ieee80211_channel *bwfm_bss2chan(struct bwfm_softc *, struct bwfm_bss_info *);
void	 bwfm_scan(struct bwfm_softc *);
void	 bwfm_connect(struct bwfm_softc *);

void	 bwfm_rx(struct bwfm_softc *, char *, size_t);
void	 bwfm_rx_event(struct bwfm_softc *, char *, size_t);
void	 bwfm_scan_node(struct bwfm_softc *, struct bwfm_bss_info *, size_t);

uint8_t bwfm_2ghz_channels[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
};
uint8_t bwfm_5ghz_channels[] = {
	34, 36, 38, 40, 42, 44, 46, 48, 52, 56, 60, 64, 100, 104, 108, 112,
	116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 161, 165,
};

struct bwfm_proto_ops bwfm_proto_bcdc_ops = {
	.proto_query_dcmd = bwfm_proto_bcdc_query_dcmd,
	.proto_set_dcmd = bwfm_proto_bcdc_set_dcmd,
};

void
bwfm_attach(struct bwfm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_if;
	struct bwfm_task *t;
	char fw_version[BWFM_DCMD_SMLEN];
	uint32_t bandlist[3];
	uint32_t tmp;
	int i, error;

	error = workqueue_create(&sc->sc_taskq, DEVNAME(sc),
	    bwfm_task, sc, PRI_NONE, IPL_NET, 0);
	if (error != 0) {
		printf("%s: could not create workqueue\n", DEVNAME(sc));
		return;
	}
	sc->sc_freetask = pcq_create(BWFM_TASK_COUNT, KM_SLEEP);
	for (i = 0; i < BWFM_TASK_COUNT; i++) {
		t = &sc->sc_task[i];
		t->t_sc = sc;
		pcq_put(sc->sc_freetask, t);
	}

	if (bwfm_fwvar_cmd_get_int(sc, BWFM_C_GET_VERSION, &tmp)) {
		printf("%s: could not read io type\n", DEVNAME(sc));
		return;
	} else
		sc->sc_io_type = tmp;
	if (bwfm_fwvar_var_get_data(sc, "cur_etheraddr", ic->ic_myaddr,
	    sizeof(ic->ic_myaddr))) {
		printf("%s: could not read mac address\n", DEVNAME(sc));
		return;
	}

	memset(fw_version, 0, sizeof(fw_version));
	if (bwfm_fwvar_var_get_data(sc, "ver", fw_version, sizeof(fw_version)) == 0)
		printf("%s: %s", DEVNAME(sc), fw_version);
	printf("%s: address %s\n", DEVNAME(sc), ether_sprintf(ic->ic_myaddr));

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_state = IEEE80211_S_INIT;

	ic->ic_caps =
	    IEEE80211_C_WEP |
	    IEEE80211_C_TKIP |
	    IEEE80211_C_AES |
	    IEEE80211_C_AES_CCM |
#if notyet
	    IEEE80211_C_MONITOR |		/* monitor mode suported */
	    IEEE80211_C_IBSS |
	    IEEE80211_C_TXPMGT |
	    IEEE80211_C_WME |
#endif
	    IEEE80211_C_SHSLOT |		/* short slot time supported */
	    IEEE80211_C_SHPREAMBLE |		/* short preamble supported */
	    IEEE80211_C_WPA |			/* 802.11i */
	    /* IEEE80211_C_WPA_4WAY */0;		/* WPA 4-way handshake in hw */

	/* IBSS channel undefined for now. */
	ic->ic_ibss_chan = &ic->ic_channels[0];

	if (bwfm_fwvar_cmd_get_data(sc, BWFM_C_GET_BANDLIST, bandlist,
	    sizeof(bandlist))) {
		printf("%s: couldn't get supported band list\n", DEVNAME(sc));
		return;
	} 
	const u_int nbands = le32toh(bandlist[0]);
	for (i = 1; i <= MIN(nbands, __arraycount(bandlist) - 1); i++) {
		switch (le32toh(bandlist[i])) {
		case BWFM_BAND_2G:
			ic->ic_sup_rates[IEEE80211_MODE_11B] = ieee80211_std_rateset_11b;
			ic->ic_sup_rates[IEEE80211_MODE_11G] = ieee80211_std_rateset_11g;

			for (i = 0; i < __arraycount(bwfm_2ghz_channels); i++) {
				uint8_t chan = bwfm_2ghz_channels[i];
				ic->ic_channels[chan].ic_freq =
				    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_2GHZ);
				ic->ic_channels[chan].ic_flags =
				    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
				    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
			}
			break;
		case BWFM_BAND_5G:
			ic->ic_sup_rates[IEEE80211_MODE_11A] = ieee80211_std_rateset_11a;

			for (i = 0; i < __arraycount(bwfm_5ghz_channels); i++) {
				uint8_t chan = bwfm_5ghz_channels[i];
				ic->ic_channels[chan].ic_freq =
				    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_5GHZ);
				ic->ic_channels[chan].ic_flags =
				    IEEE80211_CHAN_A;
			}
			break;
		}
	}

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = bwfm_init;
	ifp->if_ioctl = bwfm_ioctl;
	ifp->if_start = bwfm_start;
	ifp->if_watchdog = bwfm_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, DEVNAME(sc), IFNAMSIZ);

	error = if_initialize(ifp);
	if (error != 0) {
		printf("%s: if_initialize failed(%d)\n", DEVNAME(sc), error);
		pcq_destroy(sc->sc_freetask);
		workqueue_destroy(sc->sc_taskq);

		return; /* Error */
	}
		
	ieee80211_ifattach(ic);
	ifp->if_percpuq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);
	if_register(ifp);

	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = bwfm_newstate;
	ic->ic_newassoc = bwfm_newassoc;
	ic->ic_send_mgmt = bwfm_send_mgmt;
	ic->ic_recv_mgmt = bwfm_recv_mgmt;
	ic->ic_crypto.cs_key_set = bwfm_key_set;
	ic->ic_crypto.cs_key_delete = bwfm_key_delete;
	ieee80211_media_init(ic, bwfm_media_change, bwfm_media_status);

	ieee80211_announce(ic);

	sc->sc_if_attached = true;
}

int
bwfm_detach(struct bwfm_softc *sc, int flags)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;

	if (sc->sc_if_attached) {
		bpf_detach(ifp);
		ieee80211_ifdetach(ic);
		if_detach(ifp);
	}

	if (sc->sc_taskq)
		workqueue_destroy(sc->sc_taskq);
	if (sc->sc_freetask)
		pcq_destroy(sc->sc_freetask);

	return 0;
}

void
bwfm_start(struct ifnet *ifp)
{
	struct bwfm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct mbuf *m;
	int error;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/* TODO: return if no link? */

	for (;;) {
		struct ieee80211_node *ni;
		struct ether_header *eh;

		/* Discard management packets (fw handles this for us) */
		IF_DEQUEUE(&ic->ic_mgtq, m);
		if (m != NULL) {
			m_freem(m);
			continue;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		eh = mtod(m, struct ether_header *);
		ni = ieee80211_find_txnode(ic, eh->ether_dhost);
		if (ni == NULL) {
			ifp->if_oerrors++;
			m_freem(m);
			continue;
		}

		if (ieee80211_classify(ic, m, ni) != 0) {
			ifp->if_oerrors++;
			m_freem(m);
			ieee80211_free_node(ni);
			continue;
		}

		error = sc->sc_bus_ops->bs_txdata(sc, m);
		if (error == ENOBUFS) {
			IF_PREPEND(&ifp->if_snd, m);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		if (error != 0) {
			ifp->if_oerrors++;
			m_freem(m);
			if (ni != NULL)
				ieee80211_free_node(ni);
		} else {
			bpf_mtap3(ic->ic_rawbpf, m);
		}
	}
}

int
bwfm_init(struct ifnet *ifp)
{
	struct bwfm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t evmask[BWFM_EVENT_MASK_LEN];
	struct bwfm_join_pref_params join_pref[2];

	if (bwfm_fwvar_var_set_int(sc, "mpc", 1)) {
		printf("%s: could not set mpc\n", DEVNAME(sc));
		return EIO;
	}

	/* Select target by RSSI (boost on 5GHz) */
	join_pref[0].type = BWFM_JOIN_PREF_RSSI_DELTA;
	join_pref[0].len = 2;
	join_pref[0].rssi_gain = BWFM_JOIN_PREF_RSSI_BOOST;
	join_pref[0].band = BWFM_JOIN_PREF_BAND_5G;
	join_pref[1].type = BWFM_JOIN_PREF_RSSI;
	join_pref[1].len = 2;
	join_pref[1].rssi_gain = 0;
	join_pref[1].band = 0;
	if (bwfm_fwvar_var_set_data(sc, "join_pref", join_pref,
	    sizeof(join_pref))) {
		printf("%s: could not set join pref\n", DEVNAME(sc));
		return EIO;
	}

	memset(evmask, 0, sizeof(evmask));

#define	ENABLE_EVENT(e)		evmask[(e) / 8] |= 1 << ((e) % 8)
	/* Events used to drive the state machine */
	ENABLE_EVENT(BWFM_E_ASSOC);
	ENABLE_EVENT(BWFM_E_ESCAN_RESULT);
	ENABLE_EVENT(BWFM_E_SET_SSID);
	ENABLE_EVENT(BWFM_E_LINK);
#undef	ENABLE_EVENT

#ifdef BWFM_DEBUG
	memset(evmask, 0xff, sizeof(evmask));
#endif
	
	if (bwfm_fwvar_var_set_data(sc, "event_msgs", evmask, sizeof(evmask))) {
		printf("%s: could not set event mask\n", DEVNAME(sc));
		return EIO;
	}

	if (bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_SCAN_CHANNEL_TIME,
	    BWFM_DEFAULT_SCAN_CHANNEL_TIME)) {
		printf("%s: could not set scan channel time\n", DEVNAME(sc));
		return EIO;
	}
	if (bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_SCAN_UNASSOC_TIME,
	    BWFM_DEFAULT_SCAN_UNASSOC_TIME)) {
		printf("%s: could not set scan unassoc time\n", DEVNAME(sc));
		return EIO;
	}
	if (bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_SCAN_PASSIVE_TIME,
	    BWFM_DEFAULT_SCAN_PASSIVE_TIME)) {
		printf("%s: could not set scan passive time\n", DEVNAME(sc));
		return EIO;
	}

	if (bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_PM, 2)) {
		printf("%s: could not set power\n", DEVNAME(sc));
		return EIO;
	}

	bwfm_fwvar_var_set_int(sc, "txbf", 1);
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_UP, 0);
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_INFRA, 1);
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_AP, 0);

	/* Disable all offloading (ARP, NDP, TCP/UDP cksum). */
	bwfm_fwvar_var_set_int(sc, "arp_ol", 0);
	bwfm_fwvar_var_set_int(sc, "arpoe", 0);
	bwfm_fwvar_var_set_int(sc, "ndoe", 0);
	bwfm_fwvar_var_set_int(sc, "toe", 0);

	/*
	 * Tell the firmware supplicant that we are going to handle the
	 * WPA handshake ourselves.
	 */
	bwfm_fwvar_var_set_int(sc, "sup_wpa", 0);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (ic->ic_opmode != IEEE80211_M_MONITOR) {
		if (ic->ic_roaming != IEEE80211_ROAMING_MANUAL)
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
	} else {
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
	}

	return 0;
}

void
bwfm_stop(struct ifnet *ifp, int disable)
{
	struct bwfm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;

	sc->sc_tx_timer = 0;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	bwfm_fwvar_cmd_set_int(sc, BWFM_C_DOWN, 1);
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_PM, 0);

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
}

void
bwfm_watchdog(struct ifnet *ifp)
{
	struct bwfm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", DEVNAME(sc));
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}
	ieee80211_watchdog(ic);
}

int
bwfm_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct bwfm_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_UP | IFF_RUNNING:
			break;
		case IFF_UP:
			bwfm_init(ifp);
			break;
		case IFF_RUNNING:
			bwfm_stop(ifp, 1);
			break;
		case 0:
			break;
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/* setup multicast filter, etc */
			error = 0;
		}
		break;

	default:
		error = ieee80211_ioctl(ic, cmd, data);
	}

	if (error == ENETRESET) {
		if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0 &&
		    ic->ic_roaming != IEEE80211_ROAMING_MANUAL) {
			bwfm_init(ifp);
		}
		error = 0;
	}

	splx(s);

	return error;
}

int
bwfm_send_mgmt(struct ieee80211com *ic, struct ieee80211_node *ni,
    int type, int arg)
{
	return 0;
}

void
bwfm_recv_mgmt(struct ieee80211com *ic, struct mbuf *m0,
    struct ieee80211_node *ni, int subtype, int rssi, uint32_t rstamp)
{
}

int
bwfm_key_set(struct ieee80211com *ic, const struct ieee80211_key *wk,
    const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct bwfm_softc *sc = ic->ic_ifp->if_softc;
	struct bwfm_task *t;

	t = pcq_get(sc->sc_freetask);
	if (t == NULL) {
		printf("%s: no free tasks\n", DEVNAME(sc));
		return 0;
	}

	t->t_cmd = BWFM_TASK_KEY_SET;
	t->t_key.key = wk;
	memcpy(t->t_key.mac, mac, sizeof(t->t_key.mac));
	workqueue_enqueue(sc->sc_taskq, (struct work *)t, NULL);
	return 1;
}

static void
bwfm_key_set_cb(struct bwfm_softc *sc, struct bwfm_cmd_key *ck)
{
	const struct ieee80211_key *wk = ck->key;
	const uint8_t *mac = ck->mac;
	struct bwfm_wsec_key wsec_key;
	uint32_t wsec_enable, wsec;
	bool ext_key;

#ifdef BWFM_DEBUG
	printf("key_set: key cipher %s len %d: ", wk->wk_cipher->ic_name, wk->wk_keylen);
	for (int j = 0; j < sizeof(wk->wk_key); j++)
		printf("%02x", wk->wk_key[j]);
#endif

	if ((wk->wk_flags & IEEE80211_KEY_GROUP) == 0 &&
	    wk->wk_cipher->ic_cipher != IEEE80211_CIPHER_WEP) {
		ext_key = true;
	} else {
		ext_key = false;
	}

#ifdef BWFM_DEBUG
	printf(", ext_key = %d", ext_key);
	printf(", mac = %02x:%02x:%02x:%02x:%02x:%02x",
	    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	printf("\n");
#endif

	memset(&wsec_key, 0, sizeof(wsec_key));
	if (ext_key && !IEEE80211_IS_MULTICAST(mac))
		memcpy(wsec_key.ea, mac, sizeof(wsec_key.ea));
	wsec_key.index = htole32(wk->wk_keyix);
	wsec_key.len = htole32(wk->wk_keylen);
	memcpy(wsec_key.data, wk->wk_key, sizeof(wsec_key.data));
	if (!ext_key)
		wsec_key.flags = htole32(BWFM_PRIMARY_KEY);

	switch (wk->wk_cipher->ic_cipher) {
	case IEEE80211_CIPHER_WEP:
		if (wk->wk_keylen == 5)
			wsec_key.algo = htole32(BWFM_CRYPTO_ALGO_WEP1);
		else if (wk->wk_keylen == 13)
			wsec_key.algo = htole32(BWFM_CRYPTO_ALGO_WEP128);
		else
			return;
		wsec_enable = BWFM_WSEC_WEP;
		break;
	case IEEE80211_CIPHER_TKIP:
		wsec_key.algo = htole32(BWFM_CRYPTO_ALGO_TKIP);
		wsec_enable = BWFM_WSEC_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		wsec_key.algo = htole32(BWFM_CRYPTO_ALGO_AES_CCM);
		wsec_enable = BWFM_WSEC_AES;
		break;
	default:
		printf("%s: %s: cipher %s not supported\n", DEVNAME(sc),
		    __func__, wk->wk_cipher->ic_name);
		return;
	}

	if (bwfm_fwvar_var_set_data(sc, "wsec_key", &wsec_key, sizeof(wsec_key)))
		return;

	bwfm_fwvar_var_set_int(sc, "wpa_auth", BWFM_WPA_AUTH_WPA2_PSK);

	bwfm_fwvar_var_get_int(sc, "wsec", &wsec);
	wsec |= wsec_enable;
	bwfm_fwvar_var_set_int(sc, "wsec", wsec);
}

int
bwfm_key_delete(struct ieee80211com *ic, const struct ieee80211_key *wk)
{
	struct bwfm_softc *sc = ic->ic_ifp->if_softc;
	struct bwfm_task *t;

	t = pcq_get(sc->sc_freetask);
	if (t == NULL) {
		printf("%s: no free tasks\n", DEVNAME(sc));
		return 0;
	}

	t->t_cmd = BWFM_TASK_KEY_DELETE;
	t->t_key.key = wk;
	memset(t->t_key.mac, 0, sizeof(t->t_key.mac));
	workqueue_enqueue(sc->sc_taskq, (struct work *)t, NULL);

	return 1;
}

static void
bwfm_key_delete_cb(struct bwfm_softc *sc, struct bwfm_cmd_key *ck)
{
	const struct ieee80211_key *wk = ck->key;
	struct bwfm_wsec_key wsec_key;

	memset(&wsec_key, 0, sizeof(wsec_key));
	wsec_key.index = htole32(wk->wk_keyix);
	wsec_key.flags = htole32(BWFM_PRIMARY_KEY);

	if (bwfm_fwvar_var_set_data(sc, "wsec_key", &wsec_key, sizeof(wsec_key)))
		return;
}

int
bwfm_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct bwfm_softc *sc = ic->ic_ifp->if_softc;
	struct bwfm_task *t;

	t = pcq_get(sc->sc_freetask);
	if (t == NULL) {
		printf("%s: no free tasks\n", DEVNAME(sc));
		return EIO;
	}

	t->t_cmd = BWFM_TASK_NEWSTATE;
	t->t_newstate.state = nstate;
	t->t_newstate.arg = arg;
	workqueue_enqueue(sc->sc_taskq, (struct work *)t, NULL);

	return 0;
}

void
bwfm_newstate_cb(struct bwfm_softc *sc, struct bwfm_cmd_newstate *cmd)
{
	struct ieee80211com *ic = &sc->sc_ic;
	enum ieee80211_state ostate = ic->ic_state;
	enum ieee80211_state nstate = cmd->state;
	int s;

	DPRINTF(("%s: newstate %d -> %d\n", DEVNAME(sc), ostate, nstate));

	s = splnet();

	switch (nstate) {
	case IEEE80211_S_INIT:
		break;

	case IEEE80211_S_SCAN:
		if (ostate != IEEE80211_S_SCAN) {
			/* Start of scanning */
			bwfm_scan(sc);
		}
		break;

	case IEEE80211_S_AUTH:
		bwfm_connect(sc);
		break;

	case IEEE80211_S_ASSOC:
		break;

	case IEEE80211_S_RUN:
		break;
	}

	sc->sc_newstate(ic, nstate, cmd->arg);

	splx(s);
}

void
bwfm_newassoc(struct ieee80211_node *ni, int isnew)
{
	/* Firmware handles rate adaptation for us */
	ni->ni_txrate = 0;
}

void
bwfm_task(struct work *wk, void *arg)
{
	struct bwfm_task *t = (struct bwfm_task *)wk;
	struct bwfm_softc *sc = t->t_sc;

	switch (t->t_cmd) {
	case BWFM_TASK_NEWSTATE:
		bwfm_newstate_cb(sc, &t->t_newstate);
		break;
	case BWFM_TASK_KEY_SET:
		bwfm_key_set_cb(sc, &t->t_key);
		break;
	case BWFM_TASK_KEY_DELETE:
		bwfm_key_delete_cb(sc, &t->t_key);
		break;
	default:
		panic("bwfm: unknown task command %d", t->t_cmd);
	}

	pcq_put(sc->sc_freetask, t);
}

int
bwfm_media_change(struct ifnet *ifp)
{
	return 0;
}

void
bwfm_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
}

/* Chip initialization (SDIO, PCIe) */
int
bwfm_chip_attach(struct bwfm_softc *sc)
{
	struct bwfm_core *core;
	int need_socram = 0;
	int has_socram = 0;
	int cpu_found = 0;
	uint32_t val;

	LIST_INIT(&sc->sc_chip.ch_list);

	if (sc->sc_buscore_ops->bc_prepare(sc) != 0) {
		printf("%s: failed buscore prepare\n", DEVNAME(sc));
		return 1;
	}

	val = sc->sc_buscore_ops->bc_read(sc,
	    BWFM_CHIP_BASE + BWFM_CHIP_REG_CHIPID);
	sc->sc_chip.ch_chip = BWFM_CHIP_CHIPID_ID(val);
	sc->sc_chip.ch_chiprev = BWFM_CHIP_CHIPID_REV(val);

	if ((sc->sc_chip.ch_chip > 0xa000) || (sc->sc_chip.ch_chip < 0x4000))
		snprintf(sc->sc_chip.ch_name, sizeof(sc->sc_chip.ch_name),
		    "%d", sc->sc_chip.ch_chip);
	else
		snprintf(sc->sc_chip.ch_name, sizeof(sc->sc_chip.ch_name),
		    "%x", sc->sc_chip.ch_chip);

	switch (BWFM_CHIP_CHIPID_TYPE(val))
	{
	case BWFM_CHIP_CHIPID_TYPE_SOCI_SB:
		printf("%s: SoC interconnect SB not implemented\n",
		    DEVNAME(sc));
		return 1;
	case BWFM_CHIP_CHIPID_TYPE_SOCI_AI:
		sc->sc_chip.ch_core_isup = bwfm_chip_ai_isup;
		sc->sc_chip.ch_core_disable = bwfm_chip_ai_disable;
		sc->sc_chip.ch_core_reset = bwfm_chip_ai_reset;
		bwfm_chip_dmp_erom_scan(sc);
		break;
	default:
		printf("%s: SoC interconnect %d unknown\n",
		    DEVNAME(sc), BWFM_CHIP_CHIPID_TYPE(val));
		return 1;
	}

	LIST_FOREACH(core, &sc->sc_chip.ch_list, co_link) {
		DPRINTF(("%s: 0x%x:%-2d base 0x%08x wrap 0x%08x\n",
		    DEVNAME(sc), core->co_id, core->co_rev,
		    core->co_base, core->co_wrapbase));

		switch (core->co_id) {
		case BWFM_AGENT_CORE_ARM_CM3:
			need_socram = true;
			/* FALLTHROUGH */
		case BWFM_AGENT_CORE_ARM_CR4:
		case BWFM_AGENT_CORE_ARM_CA7:
			cpu_found = true;
			break;
		case BWFM_AGENT_INTERNAL_MEM:
			has_socram = true;
			break;
		default:
			break;
		}
	}

	if (!cpu_found) {
		printf("%s: CPU core not detected\n", DEVNAME(sc));
		return 1;
	}
	if (need_socram && !has_socram) {
		printf("%s: RAM core not provided\n", DEVNAME(sc));
		return 1;
	}

	if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CR4) != NULL)
		bwfm_chip_cr4_set_passive(sc);
	if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CA7) != NULL)
		bwfm_chip_ca7_set_passive(sc);
	if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CM3) != NULL)
		bwfm_chip_cm3_set_passive(sc);

	if (sc->sc_buscore_ops->bc_reset) {
		sc->sc_buscore_ops->bc_reset(sc);
		if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CR4) != NULL)
			bwfm_chip_cr4_set_passive(sc);
		if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CA7) != NULL)
			bwfm_chip_ca7_set_passive(sc);
		if (bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CM3) != NULL)
			bwfm_chip_cm3_set_passive(sc);
	}

	/* TODO: get raminfo */

	core = bwfm_chip_get_core(sc, BWFM_AGENT_CORE_CHIPCOMMON);
	sc->sc_chip.ch_cc_caps = sc->sc_buscore_ops->bc_read(sc,
	    core->co_base + BWFM_CHIP_REG_CAPABILITIES);
	sc->sc_chip.ch_cc_caps_ext = sc->sc_buscore_ops->bc_read(sc,
	    core->co_base + BWFM_CHIP_REG_CAPABILITIES_EXT);

	core = bwfm_chip_get_pmu(sc);
	if (sc->sc_chip.ch_cc_caps & BWFM_CHIP_REG_CAPABILITIES_PMU) {
		sc->sc_chip.ch_pmucaps = sc->sc_buscore_ops->bc_read(sc,
		    core->co_base + BWFM_CHIP_REG_PMUCAPABILITIES);
		sc->sc_chip.ch_pmurev = sc->sc_chip.ch_pmucaps &
		    BWFM_CHIP_REG_PMUCAPABILITIES_REV_MASK;
	}

	if (sc->sc_buscore_ops->bc_setup)
		sc->sc_buscore_ops->bc_setup(sc);

	return 0;
}

struct bwfm_core *
bwfm_chip_get_core(struct bwfm_softc *sc, int id)
{
	struct bwfm_core *core;

	LIST_FOREACH(core, &sc->sc_chip.ch_list, co_link) {
		if (core->co_id == id)
			return core;
	}

	return NULL;
}

struct bwfm_core *
bwfm_chip_get_pmu(struct bwfm_softc *sc)
{
	struct bwfm_core *cc, *pmu;

	cc = bwfm_chip_get_core(sc, BWFM_AGENT_CORE_CHIPCOMMON);
	if (cc->co_rev >= 35 && sc->sc_chip.ch_cc_caps_ext &
	    BWFM_CHIP_REG_CAPABILITIES_EXT_AOB_PRESENT) {
		pmu = bwfm_chip_get_core(sc, BWFM_AGENT_CORE_PMU);
		if (pmu)
			return pmu;
	}

	return cc;
}

/* Functions for the AI interconnect */
int
bwfm_chip_ai_isup(struct bwfm_softc *sc, struct bwfm_core *core)
{
	uint32_t ioctl, reset;

	ioctl = sc->sc_buscore_ops->bc_read(sc,
	    core->co_wrapbase + BWFM_AGENT_IOCTL);
	reset = sc->sc_buscore_ops->bc_read(sc,
	    core->co_wrapbase + BWFM_AGENT_RESET_CTL);

	if (((ioctl & (BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK)) ==
	    BWFM_AGENT_IOCTL_CLK) &&
	    ((reset & BWFM_AGENT_RESET_CTL_RESET) == 0))
		return 1;

	return 0;
}

void
bwfm_chip_ai_disable(struct bwfm_softc *sc, struct bwfm_core *core,
    uint32_t prereset, uint32_t reset)
{
	uint32_t val;
	int i;

	val = sc->sc_buscore_ops->bc_read(sc,
	    core->co_wrapbase + BWFM_AGENT_RESET_CTL);
	if ((val & BWFM_AGENT_RESET_CTL_RESET) == 0) {

		sc->sc_buscore_ops->bc_write(sc,
		    core->co_wrapbase + BWFM_AGENT_IOCTL,
		    prereset | BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK);
		sc->sc_buscore_ops->bc_read(sc,
		    core->co_wrapbase + BWFM_AGENT_IOCTL);

		sc->sc_buscore_ops->bc_write(sc,
		    core->co_wrapbase + BWFM_AGENT_RESET_CTL,
		    BWFM_AGENT_RESET_CTL_RESET);
		delay(20);

		for (i = 300; i > 0; i--) {
			if (sc->sc_buscore_ops->bc_read(sc,
			    core->co_wrapbase + BWFM_AGENT_RESET_CTL) ==
			    BWFM_AGENT_RESET_CTL_RESET)
				break;
		}
		if (i == 0)
			printf("%s: timeout on core reset\n", DEVNAME(sc));
	}

	sc->sc_buscore_ops->bc_write(sc,
	    core->co_wrapbase + BWFM_AGENT_IOCTL,
	    reset | BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK);
	sc->sc_buscore_ops->bc_read(sc,
	    core->co_wrapbase + BWFM_AGENT_IOCTL);
}

void
bwfm_chip_ai_reset(struct bwfm_softc *sc, struct bwfm_core *core,
    uint32_t prereset, uint32_t reset, uint32_t postreset)
{
	int i;

	bwfm_chip_ai_disable(sc, core, prereset, reset);

	for (i = 50; i > 0; i--) {
		if ((sc->sc_buscore_ops->bc_read(sc,
		    core->co_wrapbase + BWFM_AGENT_RESET_CTL) &
		    BWFM_AGENT_RESET_CTL_RESET) == 0)
			break;
		sc->sc_buscore_ops->bc_write(sc,
		    core->co_wrapbase + BWFM_AGENT_RESET_CTL, 0);
		delay(60);
	}
	if (i == 0)
		printf("%s: timeout on core reset\n", DEVNAME(sc));

	sc->sc_buscore_ops->bc_write(sc,
	    core->co_wrapbase + BWFM_AGENT_IOCTL,
	    postreset | BWFM_AGENT_IOCTL_CLK);
	sc->sc_buscore_ops->bc_read(sc,
	    core->co_wrapbase + BWFM_AGENT_IOCTL);
}

void
bwfm_chip_dmp_erom_scan(struct bwfm_softc *sc)
{
	uint32_t erom, val, base, wrap;
	uint8_t type = 0;
	uint16_t id;
	uint8_t nmw, nsw, rev;
	struct bwfm_core *core;

	erom = sc->sc_buscore_ops->bc_read(sc,
	    BWFM_CHIP_BASE + BWFM_CHIP_REG_EROMPTR);
	while (type != BWFM_DMP_DESC_EOT) {
		val = sc->sc_buscore_ops->bc_read(sc, erom);
		type = val & BWFM_DMP_DESC_MASK;
		erom += 4;

		if (type != BWFM_DMP_DESC_COMPONENT)
			continue;

		id = (val & BWFM_DMP_COMP_PARTNUM)
		    >> BWFM_DMP_COMP_PARTNUM_S;

		val = sc->sc_buscore_ops->bc_read(sc, erom);
		type = val & BWFM_DMP_DESC_MASK;
		erom += 4;

		if (type != BWFM_DMP_DESC_COMPONENT) {
			printf("%s: not component descriptor\n", DEVNAME(sc));
			return;
		}

		nmw = (val & BWFM_DMP_COMP_NUM_MWRAP)
		    >> BWFM_DMP_COMP_NUM_MWRAP_S;
		nsw = (val & BWFM_DMP_COMP_NUM_SWRAP)
		    >> BWFM_DMP_COMP_NUM_SWRAP_S;
		rev = (val & BWFM_DMP_COMP_REVISION)
		    >> BWFM_DMP_COMP_REVISION_S;

		if (nmw + nsw == 0 && id != BWFM_AGENT_CORE_PMU)
			continue;

		if (bwfm_chip_dmp_get_regaddr(sc, &erom, &base, &wrap))
			continue;

		core = kmem_alloc(sizeof(*core), KM_SLEEP);
		core->co_id = id;
		core->co_base = base;
		core->co_wrapbase = wrap;
		core->co_rev = rev;
		LIST_INSERT_HEAD(&sc->sc_chip.ch_list, core, co_link);
	}
}

int
bwfm_chip_dmp_get_regaddr(struct bwfm_softc *sc, uint32_t *erom,
    uint32_t *base, uint32_t *wrap)
{
	uint8_t type = 0, mpnum __unused = 0;
	uint8_t stype, sztype, wraptype;
	uint32_t val;

	*base = 0;
	*wrap = 0;

	val = sc->sc_buscore_ops->bc_read(sc, *erom);
	type = val & BWFM_DMP_DESC_MASK;
	if (type == BWFM_DMP_DESC_MASTER_PORT) {
		mpnum = (val & BWFM_DMP_MASTER_PORT_NUM)
		    >> BWFM_DMP_MASTER_PORT_NUM_S;
		wraptype = BWFM_DMP_SLAVE_TYPE_MWRAP;
		*erom += 4;
	} else if ((type & ~BWFM_DMP_DESC_ADDRSIZE_GT32) ==
	    BWFM_DMP_DESC_ADDRESS)
		wraptype = BWFM_DMP_SLAVE_TYPE_SWRAP;
	else
		return 1;

	do {
		do {
			val = sc->sc_buscore_ops->bc_read(sc, *erom);
			type = val & BWFM_DMP_DESC_MASK;
			if (type == BWFM_DMP_DESC_COMPONENT)
				return 0;
			if (type == BWFM_DMP_DESC_EOT)
				return 1;
			*erom += 4;
		} while ((type & ~BWFM_DMP_DESC_ADDRSIZE_GT32) !=
		     BWFM_DMP_DESC_ADDRESS);

		if (type & BWFM_DMP_DESC_ADDRSIZE_GT32)
			*erom += 4;

		sztype = (val & BWFM_DMP_SLAVE_SIZE_TYPE)
		    >> BWFM_DMP_SLAVE_SIZE_TYPE_S;
		if (sztype == BWFM_DMP_SLAVE_SIZE_DESC) {
			val = sc->sc_buscore_ops->bc_read(sc, *erom);
			type = val & BWFM_DMP_DESC_MASK;
			if (type & BWFM_DMP_DESC_ADDRSIZE_GT32)
				*erom += 8;
			else
				*erom += 4;
		}
		if (sztype != BWFM_DMP_SLAVE_SIZE_4K)
			continue;

		stype = (val & BWFM_DMP_SLAVE_TYPE) >> BWFM_DMP_SLAVE_TYPE_S;
		if (*base == 0 && stype == BWFM_DMP_SLAVE_TYPE_SLAVE)
			*base = val & BWFM_DMP_SLAVE_ADDR_BASE;
		if (*wrap == 0 && stype == wraptype)
			*wrap = val & BWFM_DMP_SLAVE_ADDR_BASE;
	} while (*base == 0 || *wrap == 0);

	return 0;
}

/* Core configuration */
void
bwfm_chip_cr4_set_passive(struct bwfm_softc *sc)
{
	panic("%s: CR4 not supported", DEVNAME(sc));
}

void
bwfm_chip_ca7_set_passive(struct bwfm_softc *sc)
{
	panic("%s: CA7 not supported", DEVNAME(sc));
}

void
bwfm_chip_cm3_set_passive(struct bwfm_softc *sc)
{
	struct bwfm_core *core;

	core = bwfm_chip_get_core(sc, BWFM_AGENT_CORE_ARM_CM3);
	sc->sc_chip.ch_core_disable(sc, core, 0, 0);
	core = bwfm_chip_get_core(sc, BWFM_AGENT_CORE_80211);
	sc->sc_chip.ch_core_reset(sc, core, BWFM_AGENT_D11_IOCTL_PHYRESET |
	    BWFM_AGENT_D11_IOCTL_PHYCLOCKEN, BWFM_AGENT_D11_IOCTL_PHYCLOCKEN,
	    BWFM_AGENT_D11_IOCTL_PHYCLOCKEN);
	core = bwfm_chip_get_core(sc, BWFM_AGENT_INTERNAL_MEM);
	sc->sc_chip.ch_core_reset(sc, core, 0, 0, 0);

	if (sc->sc_chip.ch_chip == BRCM_CC_43430_CHIP_ID) {
		sc->sc_buscore_ops->bc_write(sc,
		    core->co_base + BWFM_SOCRAM_BANKIDX, 3);
		sc->sc_buscore_ops->bc_write(sc,
		    core->co_base + BWFM_SOCRAM_BANKPDA, 0);
	}
}

/* BCDC protocol implementation */
int
bwfm_proto_bcdc_query_dcmd(struct bwfm_softc *sc, int ifidx,
    int cmd, char *buf, size_t *len)
{
	struct bwfm_proto_bcdc_dcmd *dcmd;
	size_t size = sizeof(dcmd->hdr) + *len;
	static int reqid = 0;
	int ret = 1;

	reqid++;

	dcmd = kmem_zalloc(sizeof(*dcmd), KM_SLEEP);
	if (*len > sizeof(dcmd->buf))
		goto err;

	dcmd->hdr.cmd = htole32(cmd);
	dcmd->hdr.len = htole32(*len);
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_GET;
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_ID_SET(reqid);
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_IF_SET(ifidx);
	dcmd->hdr.flags = htole32(dcmd->hdr.flags);
	memcpy(&dcmd->buf, buf, *len);

	if (sc->sc_bus_ops->bs_txctl(sc, (void *)dcmd,
	     sizeof(dcmd->hdr) + *len)) {
		DPRINTF(("%s: tx failed\n", DEVNAME(sc)));
		goto err;
	}

	do {
		if (sc->sc_bus_ops->bs_rxctl(sc, (void *)dcmd, &size)) {
			DPRINTF(("%s: rx failed\n", DEVNAME(sc)));
			goto err;
		}
		dcmd->hdr.cmd = le32toh(dcmd->hdr.cmd);
		dcmd->hdr.len = le32toh(dcmd->hdr.len);
		dcmd->hdr.flags = le32toh(dcmd->hdr.flags);
		dcmd->hdr.status = le32toh(dcmd->hdr.status);
	} while (BWFM_BCDC_DCMD_ID_GET(dcmd->hdr.flags) != reqid);

	if (BWFM_BCDC_DCMD_ID_GET(dcmd->hdr.flags) != reqid) {
		printf("%s: unexpected request id\n", DEVNAME(sc));
		goto err;
	}

	if (buf) {
		if (size > *len)
			size = *len;
		if (size < *len)
			*len = size;
		memcpy(buf, dcmd->buf, *len);
	}

	if (dcmd->hdr.flags & BWFM_BCDC_DCMD_ERROR)
		ret = dcmd->hdr.status;
	else
		ret = 0;
err:
	kmem_free(dcmd, sizeof(*dcmd));
	return ret;
}

int
bwfm_proto_bcdc_set_dcmd(struct bwfm_softc *sc, int ifidx,
    int cmd, char *buf, size_t len)
{
	struct bwfm_proto_bcdc_dcmd *dcmd;
	size_t size = sizeof(dcmd->hdr) + len;
	int reqid = 0;
	int ret = 1;

	reqid++;

	dcmd = kmem_zalloc(sizeof(*dcmd), KM_SLEEP);
	if (len > sizeof(dcmd->buf))
		goto err;

	dcmd->hdr.cmd = htole32(cmd);
	dcmd->hdr.len = htole32(len);
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_SET;
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_ID_SET(reqid);
	dcmd->hdr.flags |= BWFM_BCDC_DCMD_IF_SET(ifidx);
	dcmd->hdr.flags = htole32(dcmd->hdr.flags);
	memcpy(&dcmd->buf, buf, len);

	if (sc->sc_bus_ops->bs_txctl(sc, (void *)dcmd, size)) {
		DPRINTF(("%s: tx failed\n", DEVNAME(sc)));
		goto err;
	}

	do {
		if (sc->sc_bus_ops->bs_rxctl(sc, (void *)dcmd, &size)) {
			DPRINTF(("%s: rx failed\n", DEVNAME(sc)));
			goto err;
		}
		dcmd->hdr.cmd = le32toh(dcmd->hdr.cmd);
		dcmd->hdr.len = le32toh(dcmd->hdr.len);
		dcmd->hdr.flags = le32toh(dcmd->hdr.flags);
		dcmd->hdr.status = le32toh(dcmd->hdr.status);
	} while (BWFM_BCDC_DCMD_ID_GET(dcmd->hdr.flags) != reqid);

	if (BWFM_BCDC_DCMD_ID_GET(dcmd->hdr.flags) != reqid) {
		printf("%s: unexpected request id\n", DEVNAME(sc));
		goto err;
	}

	if (dcmd->hdr.flags & BWFM_BCDC_DCMD_ERROR)
		return dcmd->hdr.status;

	ret = 0;
err:
	kmem_free(dcmd, sizeof(*dcmd));
	return ret;
}

/* FW Variable code */
int
bwfm_fwvar_cmd_get_data(struct bwfm_softc *sc, int cmd, void *data, size_t len)
{
	return sc->sc_proto_ops->proto_query_dcmd(sc, 0, cmd, data, &len);
}

int
bwfm_fwvar_cmd_set_data(struct bwfm_softc *sc, int cmd, void *data, size_t len)
{
	return sc->sc_proto_ops->proto_set_dcmd(sc, 0, cmd, data, len);
}

int
bwfm_fwvar_cmd_get_int(struct bwfm_softc *sc, int cmd, uint32_t *data)
{
	int ret;
	ret = bwfm_fwvar_cmd_get_data(sc, cmd, data, sizeof(*data));
	*data = le32toh(*data);
	return ret;
}

int
bwfm_fwvar_cmd_set_int(struct bwfm_softc *sc, int cmd, uint32_t data)
{
	data = htole32(data);
	return bwfm_fwvar_cmd_set_data(sc, cmd, &data, sizeof(data));
}

int
bwfm_fwvar_var_get_data(struct bwfm_softc *sc, const char *name, void *data, size_t len)
{
	char *buf;
	int ret;

	buf = kmem_alloc(strlen(name) + 1 + len, KM_SLEEP);
	memcpy(buf, name, strlen(name) + 1);
	memcpy(buf + strlen(name) + 1, data, len);
	ret = bwfm_fwvar_cmd_get_data(sc, BWFM_C_GET_VAR,
	    buf, strlen(name) + 1 + len);
	memcpy(data, buf, len);
	kmem_free(buf, strlen(name) + 1 + len);
	return ret;
}

int
bwfm_fwvar_var_set_data(struct bwfm_softc *sc, const char *name, void *data, size_t len)
{
	char *buf;
	int ret;

	buf = kmem_alloc(strlen(name) + 1 + len, KM_SLEEP);
	memcpy(buf, name, strlen(name) + 1);
	memcpy(buf + strlen(name) + 1, data, len);
	ret = bwfm_fwvar_cmd_set_data(sc, BWFM_C_SET_VAR,
	    buf, strlen(name) + 1 + len);
	kmem_free(buf, strlen(name) + 1 + len);
	return ret;
}

int
bwfm_fwvar_var_get_int(struct bwfm_softc *sc, const char *name, uint32_t *data)
{
	int ret;
	ret = bwfm_fwvar_var_get_data(sc, name, data, sizeof(*data));
	*data = le32toh(*data);
	return ret;
}

int
bwfm_fwvar_var_set_int(struct bwfm_softc *sc, const char *name, uint32_t data)
{
	data = htole32(data);
	return bwfm_fwvar_var_set_data(sc, name, &data, sizeof(data));
}

/* 802.11 code */
void
bwfm_scan(struct bwfm_softc *sc)
{
	struct bwfm_escan_params *params;
	uint32_t nssid = 0, nchannel = 0;
	size_t params_size;

#if 0
	/* Active scan is used for scanning for an SSID */
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_PASSIVE_SCAN, 0);
#endif
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_PASSIVE_SCAN, 1);

	params_size = sizeof(*params);
	params_size += sizeof(uint32_t) * ((nchannel + 1) / 2);
	params_size += sizeof(struct bwfm_ssid) * nssid;

	params = kmem_zalloc(params_size, KM_SLEEP);
	memset(params->scan_params.bssid, 0xff,
	    sizeof(params->scan_params.bssid));
	params->scan_params.bss_type = 2;
	params->scan_params.nprobes = htole32(-1);
	params->scan_params.active_time = htole32(-1);
	params->scan_params.passive_time = htole32(-1);
	params->scan_params.home_time = htole32(-1);
	params->version = htole32(BWFM_ESCAN_REQ_VERSION);
	params->action = htole16(WL_ESCAN_ACTION_START);
	params->sync_id = htole16(0x1234);

#if 0
	/* Scan a specific channel */
	params->scan_params.channel_list[0] = htole16(
	    (1 & 0xff) << 0 |
	    (3 & 0x3) << 8 |
	    (2 & 0x3) << 10 |
	    (2 & 0x3) << 12
	    );
	params->scan_params.channel_num = htole32(
	    (1 & 0xffff) << 0
	    );
#endif

	bwfm_fwvar_var_set_data(sc, "escan", params, params_size);
	kmem_free(params, params_size);
}

static __inline int
bwfm_iswpaoui(const uint8_t *frm)
{
	return frm[1] > 3 && le32dec(frm+2) == ((WPA_OUI_TYPE<<24)|WPA_OUI);
}

/*
 * Derive wireless security settings from WPA/RSN IE.
 */
static uint32_t
bwfm_get_wsec(struct bwfm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t *wpa = ic->ic_opt_ie;

	KASSERT(ic->ic_opt_ie_len > 0);

	if (wpa[0] != IEEE80211_ELEMID_RSN) {
		if (ic->ic_opt_ie_len < 12)
			return BWFM_WSEC_NONE;

		/* non-RSN IE, expect that we are doing WPA1 */
		if ((ic->ic_flags & IEEE80211_F_WPA1) == 0)
			return BWFM_WSEC_NONE;

		/* Must contain WPA OUI */
		if (!bwfm_iswpaoui(wpa))
			return BWFM_WSEC_NONE;

		switch (le32dec(wpa + 8)) {
		case ((WPA_CSE_TKIP<<24)|WPA_OUI):
			return BWFM_WSEC_TKIP;
		case ((WPA_CSE_CCMP<<24)|WPA_OUI):
			return BWFM_WSEC_AES;
		default:
			return BWFM_WSEC_NONE;
		}
	} else {
		if (ic->ic_opt_ie_len < 14)
			return BWFM_WSEC_NONE;

		/* RSN IE, expect that we are doing WPA2 */
		if ((ic->ic_flags & IEEE80211_F_WPA2) == 0)
			return BWFM_WSEC_NONE;

		switch (le32dec(wpa + 10)) {
		case ((RSN_CSE_TKIP<<24)|RSN_OUI):
			return BWFM_WSEC_TKIP;
		case ((RSN_CSE_CCMP<<24)|RSN_OUI):
			return BWFM_WSEC_AES;
		default:
			return BWFM_WSEC_NONE;
		}
	}
}

void
bwfm_connect(struct bwfm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct bwfm_ext_join_params *params;

	if (ic->ic_flags & IEEE80211_F_WPA) {
		uint32_t wsec = 0;
		uint32_t wpa = 0;

		if (ic->ic_opt_ie_len)
			bwfm_fwvar_var_set_data(sc, "wpaie", ic->ic_opt_ie, ic->ic_opt_ie_len);

		if (ic->ic_flags & IEEE80211_F_WPA1)
			wpa |= BWFM_WPA_AUTH_WPA_PSK;
		if (ic->ic_flags & IEEE80211_F_WPA2)
			wpa |= BWFM_WPA_AUTH_WPA2_PSK;

		wsec |= bwfm_get_wsec(sc);

		DPRINTF(("%s: WPA enabled, ic_flags = 0x%x, wpa 0x%x, wsec 0x%x\n",
		    DEVNAME(sc), ic->ic_flags, wpa, wsec));

		bwfm_fwvar_var_set_int(sc, "wpa_auth", wpa);
		bwfm_fwvar_var_set_int(sc, "wsec", wsec);
	} else {
		bwfm_fwvar_var_set_int(sc, "wpa_auth", BWFM_WPA_AUTH_DISABLED);
		bwfm_fwvar_var_set_int(sc, "wsec", BWFM_WSEC_NONE);
	}

	bwfm_fwvar_var_set_int(sc, "auth", BWFM_AUTH_OPEN);
	bwfm_fwvar_var_set_int(sc, "mfp", BWFM_MFP_NONE);

	if (ni->ni_esslen && ni->ni_esslen < BWFM_MAX_SSID_LEN) {
		params = kmem_zalloc(sizeof(*params), KM_SLEEP);
		memcpy(params->ssid.ssid, ni->ni_essid, ni->ni_esslen);
		params->ssid.len = htole32(ni->ni_esslen);
		memcpy(params->assoc.bssid, ni->ni_bssid, sizeof(params->assoc.bssid));
		params->scan.scan_type = -1;
		params->scan.nprobes = htole32(-1);
		params->scan.active_time = htole32(-1);
		params->scan.passive_time = htole32(-1);
		params->scan.home_time = htole32(-1);
		if (bwfm_fwvar_var_set_data(sc, "join", params, sizeof(*params))) {
			struct bwfm_join_params join;
			memset(&join, 0, sizeof(join));
			memcpy(join.ssid.ssid, ni->ni_essid, ni->ni_esslen);
			join.ssid.len = htole32(ni->ni_esslen);
			memcpy(join.assoc.bssid, ni->ni_bssid, sizeof(join.assoc.bssid));
			bwfm_fwvar_cmd_set_data(sc, BWFM_C_SET_SSID, &join,
			    sizeof(join));
		}
		kmem_free(params, sizeof(*params));
	}

	/* XXX: added for testing only, remove */
	bwfm_fwvar_var_set_int(sc, "allmulti", 1);
#if 0
	bwfm_fwvar_cmd_set_int(sc, BWFM_C_SET_PROMISC, 1);
#endif
}

void
bwfm_rx(struct bwfm_softc *sc, char *buf, size_t len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = ic->ic_ifp;
	struct bwfm_event *e = (void *)buf;
	struct mbuf *m;
	char *mb;
	int s;

	DPRINTF(("%s: buf %p len %lu\n", __func__, buf, len));

	if (len >= sizeof(e->ehdr) &&
	    ntohs(e->ehdr.ether_type) == BWFM_ETHERTYPE_LINK_CTL &&
	    memcmp(BWFM_BRCM_OUI, e->hdr.oui, sizeof(e->hdr.oui)) == 0 &&
	    ntohs(e->hdr.usr_subtype) == BWFM_BRCM_SUBTYPE_EVENT)
		bwfm_rx_event(sc, buf, len);

	if (__predict_false(len > MCLBYTES || len == 0))
		return;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (__predict_false(m == NULL))
		return;
	if (len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if (!(m->m_flags & M_EXT)) {
			m_free(m);
			return;
		}
	}

	s = splnet();

	if ((ifp->if_flags & IFF_RUNNING) != 0) {
		mb = mtod(m, char *);
		memcpy(mb, buf, len);
		m->m_pkthdr.len = m->m_len = len;
		m_set_rcvif(m, ifp);

		if_percpuq_enqueue(ifp->if_percpuq, m);
	}

	splx(s);
}

void
bwfm_rx_event(struct bwfm_softc *sc, char *buf, size_t len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct bwfm_event *e = (void *)buf;
	int s;

	DPRINTF(("%s: buf %p len %lu datalen %u code %u status %u"
	    " reason %u\n", __func__, buf, len, ntohl(e->msg.datalen),
	    ntohl(e->msg.event_type), ntohl(e->msg.status),
	    ntohl(e->msg.reason)));

	if (ntohl(e->msg.event_type) >= BWFM_E_LAST)
		return;

	switch (ntohl(e->msg.event_type)) {
	case BWFM_E_ESCAN_RESULT: {
		struct bwfm_escan_results *res = (void *)(buf + sizeof(*e));
		struct bwfm_bss_info *bss;
		int i;
		if (ntohl(e->msg.status) != BWFM_E_STATUS_PARTIAL) {
			/* Scan complete */
			s = splnet();
			if (ic->ic_opmode != IEEE80211_M_MONITOR)
				ieee80211_end_scan(ic);
			splx(s);
			break;
		}
		len -= sizeof(*e);
		if (len < sizeof(*res) || len < le32toh(res->buflen)) {
			printf("%s: results too small\n", DEVNAME(sc));
			return;
		}
		len -= sizeof(*res);
		if (len < le16toh(res->bss_count) * sizeof(struct bwfm_bss_info)) {
			printf("%s: results too small\n", DEVNAME(sc));
			return;
		}
		bss = &res->bss_info[0];
		for (i = 0; i < le16toh(res->bss_count); i++) {
			/* Fix alignment of bss_info */
			union {
				struct bwfm_bss_info bss_info;
				uint8_t padding[BWFM_BSS_INFO_BUFLEN];
			} bss_buf;
			if (len > sizeof(bss_buf)) {
				printf("%s: bss_info buffer too big\n", DEVNAME(sc));
			} else {
				memcpy(&bss_buf, &res->bss_info[i], len);
				bwfm_scan_node(sc, &bss_buf.bss_info, len);
			}
			len -= sizeof(*bss) + le32toh(bss->length);
			bss = (void *)(((uintptr_t)bss) + le32toh(bss->length));
			if (len <= 0)
				break;
		}
		break;
		}

	case BWFM_E_SET_SSID:
		if (ntohl(e->msg.status) == BWFM_E_STATUS_SUCCESS) {
			ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
		} else {
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		}
		break;

	case BWFM_E_ASSOC:
		if (ntohl(e->msg.status) == BWFM_E_STATUS_SUCCESS) {
			ieee80211_new_state(ic, IEEE80211_S_ASSOC, -1);
		} else {
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		}
		break;

	case BWFM_E_LINK:
		if (ntohl(e->msg.status) == BWFM_E_STATUS_SUCCESS &&
		    ntohl(e->msg.reason) == 0)
			break;
		
		/* Link status has changed */
		ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		break;

	default:
		break;
	}
}

void
bwfm_scan_node(struct bwfm_softc *sc, struct bwfm_bss_info *bss, size_t len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_frame wh;
	struct ieee80211_scanparams scan;
	uint8_t rates[sizeof(bss->rates) + 2];
	uint8_t ssid[sizeof(bss->ssid) + 2];
	uint8_t *frm, *sfrm, *efrm;
	uint64_t tsf;

	tsf = 0;
	sfrm = ((uint8_t *)bss) + le16toh(bss->ie_offset);
	efrm = sfrm + le32toh(bss->ie_length);

	/* Fake a wireless header with the scan result's BSSID */
	memset(&wh, 0, sizeof(wh));
	IEEE80211_ADDR_COPY(wh.i_addr2, bss->bssid);
	IEEE80211_ADDR_COPY(wh.i_addr3, bss->bssid);

	if (efrm - sfrm < 12) {
		ic->ic_stats.is_rx_elem_toosmall++;
		return;
	}

	rates[0] = 0;
	rates[1] = le32toh(bss->nrates);
	memcpy(&rates[2], bss->rates, sizeof(bss->rates));

	ssid[0] = 0;
	ssid[1] = bss->ssid_len;
	memcpy(&ssid[2], bss->ssid, sizeof(bss->ssid));

	/* Build scan result */
	memset(&scan, 0, sizeof(scan));
	scan.tstamp  = (uint8_t *)&tsf;
	scan.bintval = le16toh(bss->beacon_period);
	scan.capinfo = le16toh(bss->capability);
	scan.bchan   = ieee80211_chan2ieee(ic, ic->ic_curchan);
	scan.chan    = scan.bchan;
	scan.rates   = rates;
	scan.ssid    = ssid;

	for (frm = sfrm; frm < efrm; frm += frm[1] + 2) {
		switch (frm[0]) {
		case IEEE80211_ELEMID_COUNTRY:
			scan.country = frm;
			break;
		case IEEE80211_ELEMID_FHPARMS:
			if (ic->ic_phytype == IEEE80211_T_FH) {
				scan.fhdwell = le16dec(&frm[2]);
				scan.chan = IEEE80211_FH_CHAN(frm[4], frm[5]);
				scan.fhindex = frm[6];
			}
			break;
		case IEEE80211_ELEMID_DSPARMS:
			if (ic->ic_phytype != IEEE80211_T_FH)
				scan.chan = frm[2];
			break;
		case IEEE80211_ELEMID_TIM:
			scan.tim = frm;
			scan.timoff = frm - sfrm;
			break;
		case IEEE80211_ELEMID_XRATES:
			scan.xrates = frm;
			break;
		case IEEE80211_ELEMID_ERP:
			if (frm[1] != 1) {
				ic->ic_stats.is_rx_elem_toobig++;
				break;
			}
			scan.erp = frm[2];
			break;
		case IEEE80211_ELEMID_RSN:
			scan.wpa = frm;
			break;
		case IEEE80211_ELEMID_VENDOR:
			if (bwfm_iswpaoui(frm))
				scan.wpa = frm;
			break;
		}
	}

	if (ic->ic_flags & IEEE80211_F_SCAN)
		ieee80211_add_scan(ic, &scan, &wh, IEEE80211_FC0_SUBTYPE_BEACON,
		    le32toh(bss->rssi), 0);
}
