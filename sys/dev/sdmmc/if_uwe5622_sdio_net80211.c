/* $NetBSD$ */

/*-
 * Copyright (c) 2026 berte
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/stdint.h>
#include <sys/endian.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/bpf.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_proto.h>
#include <net80211/ieee80211_node.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/led.h>

#include "if_uwe5622_sdiovar.h"



bool
uwe5622_sdio_parse_mgmt_ies(const uint8_t *ies, size_t ies_len, char *ssid,
    size_t ssid_len, int *chanp)
{
	size_t off;

	if (ssid_len == 0)
		return false;

	ssid[0] = '\0';
	if (chanp != NULL)
		*chanp = -1;

	for (off = 0; off + 2 <= ies_len; ) {
		uint8_t eid, elen;

		eid = ies[off];
		elen = ies[off + 1];
		off += 2;
		if (off + elen > ies_len)
			break;

		switch (eid) {
		case IEEE80211_ELEMID_SSID:
			if (elen >= ssid_len)
				elen = ssid_len - 1;
			memcpy(ssid, ies + off, elen);
			ssid[elen] = '\0';
			break;
		case IEEE80211_ELEMID_DSPARMS:
			if (chanp != NULL && elen >= 1)
				*chanp = ies[off];
			break;
		case IEEE80211_ELEMID_HTINFO:
			/*
			 * 5GHz APs commonly omit DSPARMS (legacy/2.4GHz-era
			 * IE); fall back to the HT Information element's
			 * Primary Channel field, present on virtually all
			 * 11n/ac networks on both bands.
			 */
			if (chanp != NULL && *chanp == -1 && elen >= 1)
				*chanp = ies[off];
			break;
		default:
			break;
		}
		off += elen;
	}

	return ssid[0] != '\0' || (chanp != NULL && *chanp != -1);
}

void
uwe5622_sdio_try_log_mgmt_frame(struct uwe5622_sdio_softc *sc,
    const uint8_t *payload, size_t len)
{
	size_t off, minlen;
	char ssid[IEEE80211_NWID_LEN + 1];
	int chan;

	minlen = sizeof(struct uwe_sprdwl_cmd_hdr) + 24 + 12 + 2;
	if (len < minlen)
		return;

	for (off = sizeof(struct uwe_sprdwl_cmd_hdr); off + 24 + 12 + 2 <= len;
	    off++) {
		const uint8_t *frm, *ies;
		size_t ies_len;
		uint8_t fc0, fc1;
		int8_t rssi;

		frm = payload + off;
		fc0 = frm[0];
		fc1 = frm[1];

		if ((fc0 & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_MGT)
			continue;
		if ((fc0 & IEEE80211_FC0_SUBTYPE_MASK) != IEEE80211_FC0_SUBTYPE_BEACON &&
		    (fc0 & IEEE80211_FC0_SUBTYPE_MASK) != IEEE80211_FC0_SUBTYPE_PROBE_RESP)
			continue;
		if ((fc1 & IEEE80211_FC1_DIR_MASK) != IEEE80211_FC1_DIR_NODS)
			continue;

		ies = frm + 24 + 12;
		ies_len = len - (off + 24 + 12);
		chan = -1;
		(void)uwe5622_sdio_parse_mgmt_ies(ies, ies_len, ssid, sizeof(ssid),
		    &chan);
		rssi = -40;
		if (off >= sizeof(struct uwe_sprdwl_cmd_hdr) + 2)
			rssi = (int8_t)payload[off - 2];

		if (sc->sc_verbose)
			aprint_normal_dev(sc->sc_dev,
			    "mgmt parsed: %s bssid=%s sa=%s chan=%d "
			    "ssid='%s' frame_off=%zu\n",
			    ((fc0 & IEEE80211_FC0_SUBTYPE_MASK) ==
			    IEEE80211_FC0_SUBTYPE_BEACON) ?
			    "beacon" : "probe-resp",
			    ether_sprintf(frm + 16), ether_sprintf(frm + 10),
			    chan, ssid, off);
		uwe5622_sdio_publish_scan_result(sc, frm, len - off, chan, rssi);
		return;
	}
}

void
uwe5622_sdio_begin_net80211_scan(struct uwe5622_sdio_softc *sc)
{
	if (!sc->sc_net80211_attached)
		return;
	if (sc->sc_scan_publish_active)
		return;
	sc->sc_scan_publish_active = true;
	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev, "net80211: begin_scan\n");
	ieee80211_begin_scan(&sc->sc_ic, 1);
}

void
uwe5622_sdio_finish_net80211_scan(struct uwe5622_sdio_softc *sc)
{
	if (!sc->sc_net80211_attached)
		return;
	if (!sc->sc_scan_publish_active)
		return;
	sc->sc_scan_publish_active = false;
	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev, "net80211: end_scan\n");
	ieee80211_end_scan(&sc->sc_ic);
}

void
uwe5622_sdio_publish_scan_result(struct uwe5622_sdio_softc *sc,
    const uint8_t *frm, size_t frm_len, int chan, int rssi)
{
	struct ieee80211com *ic;
	struct ieee80211_node *ni;
	const struct ieee80211_frame *wh;
	const uint8_t *ies;
	const uint8_t *wpa_ie;
	const uint8_t *rates_ie;
	const uint8_t *xrates_ie;
	size_t ies_len;
	size_t ieoff;
	char ssid[IEEE80211_NWID_LEN + 1];
	int parsed_chan;
	int ieee;
	bool is_new;

	if (!sc->sc_scan_publish_active)
		return;
	if (frm_len < 24 + 12)
		return;

	ic = &sc->sc_ic;
	wh = (const struct ieee80211_frame *)frm;
	ies = frm + 24 + 12;
	ies_len = frm_len - (24 + 12);
	wpa_ie = NULL;
	rates_ie = NULL;
	xrates_ie = NULL;
	for (ieoff = 0; ieoff + 2 <= ies_len; ) {
		size_t ielen = (size_t)ies[ieoff + 1] + 2;

		if (ieoff + ielen > ies_len)
			break;
		if (ies[ieoff] == IEEE80211_ELEMID_RSN ||
		    (ies[ieoff] == IEEE80211_ELEMID_VENDOR && ielen >= 6 &&
		    ies[ieoff + 2] == 0x00 && ies[ieoff + 3] == 0x50 &&
		    ies[ieoff + 4] == 0xf2 && ies[ieoff + 5] == 0x01))
			wpa_ie = ies + ieoff;
		if (ies[ieoff] == IEEE80211_ELEMID_RATES)
			rates_ie = ies + ieoff;
		else if (ies[ieoff] == IEEE80211_ELEMID_XRATES)
			xrates_ie = ies + ieoff;
		ieoff += ielen;
	}
	parsed_chan = chan;
	ssid[0] = ' ';
	(void)uwe5622_sdio_parse_mgmt_ies(ies, ies_len, ssid, sizeof(ssid),
	    &parsed_chan);
	if (parsed_chan < 0)
		parsed_chan = 1;
	ieee = parsed_chan;
	if (ieee < 0 || ieee > IEEE80211_CHAN_MAX ||
	    ic->ic_channels[ieee].ic_freq == 0)
		ieee = 1;

	/*
	 * ieee80211_find_node() takes an extra reference for the caller (we
	 * must release it below); ieee80211_dup_bss() does not - the node it
	 * creates and inserts into the table starts at refcnt=1, which IS
	 * the table's own baseline ownership. Calling ieee80211_free_node()
	 * unconditionally after either call (as this used to) drops that
	 * baseline ref to 0 for every newly-seen BSSID, which immediately
	 * removes the node from the table again - so ic_scan always ended up
	 * empty by the time anything tried to look a node up afterward.
	 */
	ni = ieee80211_find_node(&ic->ic_scan, wh->i_addr3);
	if (ni != NULL) {
		is_new = false;
	} else {
		ni = ieee80211_dup_bss(&ic->ic_scan, wh->i_addr3);
		is_new = true;
	}
	if (ni == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "net80211: scan publish alloc failed for %s\n",
		    ether_sprintf(wh->i_addr3));
		return;
	}

	IEEE80211_ADDR_COPY(ni->ni_macaddr, wh->i_addr2);
	IEEE80211_ADDR_COPY(ni->ni_bssid, wh->i_addr3);
	ni->ni_chan = &ic->ic_channels[ieee];
	ni->ni_rssi = rssi;
	ni->ni_rstamp = getticks();
	ni->ni_intval = le16toh(*(const uint16_t *)(frm + 24 + 8));
	ni->ni_capinfo = le16toh(*(const uint16_t *)(frm + 24 + 10));
	if (ssid[0] != ' ') {
		ni->ni_esslen = uimin(strlen(ssid), IEEE80211_NWID_LEN);
		memcpy(ni->ni_essid, ssid, ni->ni_esslen);
	}
	ni->ni_rates.rs_nrates = 0;
	if (rates_ie != NULL) {
		size_t nr = uimin(rates_ie[1], IEEE80211_RATE_MAXSIZE);

		memcpy(ni->ni_rates.rs_rates, rates_ie + 2, nr);
		ni->ni_rates.rs_nrates = nr;
	}
	if (xrates_ie != NULL && ni->ni_rates.rs_nrates < IEEE80211_RATE_MAXSIZE) {
		size_t room = IEEE80211_RATE_MAXSIZE - ni->ni_rates.rs_nrates;
		size_t nr = uimin(xrates_ie[1], room);

		memcpy(ni->ni_rates.rs_rates + ni->ni_rates.rs_nrates,
		    xrates_ie + 2, nr);
		ni->ni_rates.rs_nrates += nr;
	}
	/*
	 * SIOCG80211ALLNODES exposes ni_wpa_ie to wpa_supplicant.  Merely
	 * publishing the SSID/BSSID makes a protected AP look open, causing
	 * wpa_supplicant to reject it and scan forever.  Preserve the complete
	 * RSN (or legacy WPA) element from the firmware's beacon/probe response.
	 */
	if (wpa_ie != NULL)
		ieee80211_saveie(&ni->ni_wpa_ie, wpa_ie);
	else if (ni->ni_wpa_ie != NULL) {
		free(ni->ni_wpa_ie, M_DEVBUF);
		ni->ni_wpa_ie = NULL;
	}
	if (sc->sc_verbose && strcmp(ssid, "berte-test") == 0) {
		aprint_normal_dev(sc->sc_dev,
		    "scan target: bssid=%s cap=0x%04x rates=%u security=%s "
		    "ie_len=%u\n", ether_sprintf(ni->ni_bssid), ni->ni_capinfo,
		    ni->ni_rates.rs_nrates,
		    wpa_ie != NULL ?
		    (wpa_ie[0] == IEEE80211_ELEMID_RSN ? "RSN" : "WPA") :
		    "open", wpa_ie != NULL ? wpa_ie[1] + 2 : 0);
	}

	aprint_debug_dev(sc->sc_dev,
	    "net80211: published scan result bssid=%s chan=%d rssi=%d "
	    "ssid='%s' security=%s\n",
	    ether_sprintf(ni->ni_bssid), ieee, rssi, ssid,
	    wpa_ie != NULL ?
	    (wpa_ie[0] == IEEE80211_ELEMID_RSN ? "RSN" : "WPA") : "open");
	if (!is_new)
		ieee80211_free_node(ni);
}

int
uwe5622_sdio_media_change(struct ifnet *ifp)
{
	struct uwe5622_sdio_softc *sc;
	int error;

	sc = ifp->if_softc;
	aprint_debug_dev(sc->sc_dev,
	    "media_change enter: flags=0x%x timer=%d\n",
	    ifp->if_flags, ifp->if_timer);
	error = ieee80211_media_change(ifp);
	/* Firmware is already kept in STA/autoselect mode. */
	if (error == ENETRESET)
		error = 0;
	aprint_debug_dev(sc->sc_dev,
	    "media_change exit: error=%d flags=0x%x timer=%d\n",
	    error, ifp->if_flags, ifp->if_timer);
	return error;
}

void
uwe5622_sdio_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct uwe5622_sdio_softc *sc;

	sc = ifp->if_softc;
	ieee80211_media_status(ifp, imr);
	aprint_debug_dev(sc->sc_dev,
	    "media_status: status=0x%x active=0x%x current=0x%x\n",
	    imr->ifm_status, imr->ifm_active, imr->ifm_current);
}

int
uwe5622_sdio_if_init(struct ifnet *ifp)
{
	struct uwe5622_sdio_softc *sc;

	sc = ifp->if_softc;
	aprint_debug_dev(sc->sc_dev,
	    "if_init enter: flags=0x%x timer=%d\n",
	    ifp->if_flags, ifp->if_timer);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	aprint_debug_dev(sc->sc_dev,
	    "if_init exit: flags=0x%x timer=%d\n",
	    ifp->if_flags, ifp->if_timer);
	return 0;
}

void
uwe5622_sdio_if_stop(struct ifnet *ifp, int disable)
{
	struct uwe5622_sdio_softc *sc;

	sc = ifp->if_softc;
	aprint_debug_dev(sc->sc_dev,
	    "if_stop: disable=%d flags_before=0x%x timer=%d\n",
	    disable, ifp->if_flags, ifp->if_timer);
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	if_link_state_change(ifp, LINK_STATE_DOWN);
	aprint_debug_dev(sc->sc_dev,
	    "if_stop done: flags_after=0x%x timer=%d\n",
	    ifp->if_flags, ifp->if_timer);
}

int
uwe5622_sdio_if_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct uwe5622_sdio_softc *sc;
	struct ieee80211req_mlme mlme;
	struct ieee80211req *ireq;
	char assoc_ssid[IEEE80211_NWID_LEN + 1];
	static const uint8_t zero_bssid[IEEE80211_ADDR_LEN];
	bool fw_assoc;
	int error;

	sc = ifp->if_softc;
	fw_assoc = false;
	memset(&mlme, 0, sizeof(mlme));
	/*
	 * The BSD wpa_supplicant backend selects a BSS with
	 * SIOCS80211/IEEE80211_IOC_MLME, not SIOCS80211NWID.  net80211's
	 * normal sta_join updates its software state, but this full-MAC device
	 * still needs a matching firmware CONNECT command.
	 */
	if (cmd == SIOCS80211) {
		ireq = data;
		if (ireq->i_type == IEEE80211_IOC_MLME &&
		    ireq->i_len == sizeof(mlme) &&
		    copyin(ireq->i_data, &mlme, sizeof(mlme)) == 0 &&
		    mlme.im_op == IEEE80211_MLME_ASSOC &&
		    mlme.im_ssid_len > 0 &&
		    mlme.im_ssid_len <= IEEE80211_NWID_LEN)
			fw_assoc = true;
	}

	aprint_debug_dev(sc->sc_dev,
	    "if_ioctl enter: cmd=0x%lx(%s) flags=0x%x timer=%d running=%s\n",
	    cmd, uwe5622_sdio_ioctl_name(cmd), ifp->if_flags, ifp->if_timer,
	    (ifp->if_flags & IFF_RUNNING) ? "yes" : "no");
	error = ieee80211_ioctl(&sc->sc_ic, cmd, data);
	if (error == 0 && fw_assoc && UWE_ENABLE_AUTO_WIFI_SESSION) {
		uint8_t channel = 0;
		const uint8_t *bssid = NULL;

		memcpy(assoc_ssid, mlme.im_ssid, mlme.im_ssid_len);
		assoc_ssid[mlme.im_ssid_len] = '\0';
		if (!IEEE80211_ADDR_EQ(mlme.im_macaddr, zero_bssid))
			bssid = mlme.im_macaddr;
		if (sc->sc_ic.ic_bss != NULL &&
		    sc->sc_ic.ic_bss->ni_chan != NULL &&
		    sc->sc_ic.ic_bss->ni_chan != IEEE80211_CHAN_ANYC)
			channel = sc->sc_ic.ic_bss->ni_chan -
			    sc->sc_ic.ic_channels;
		aprint_normal_dev(sc->sc_dev,
		    "bridge: wpa_supplicant MLME_ASSOC ssid='%s' bssid=%s "
		    "chan=%u\n", assoc_ssid,
		    bssid != NULL ? ether_sprintf(bssid) : "any", channel);
		error = uwe5622_sdio_wifi_cmd_connect(sc, sc->sc_wifi_ctx_id,
		    bssid, channel, assoc_ssid, "");
	}

	if (error == 0 && cmd == 0x802069f4 && UWE_ENABLE_AUTO_WIFI_SESSION) {
		aprint_debug_dev(sc->sc_dev,
		    "if_ioctl: bridging raw scan trigger into firmware scan\n");
		(void)uwe5622_sdio_bridge_scan_request(sc);
	}

	aprint_debug_dev(sc->sc_dev,
	    "if_ioctl exit: cmd=0x%lx(%s) error=%d flags=0x%x timer=%d\n",
	    cmd, uwe5622_sdio_ioctl_name(cmd), error, ifp->if_flags, ifp->if_timer);
	return error;
}

int
uwe5622_sdio_key_set(struct ieee80211com *ic,
    const struct ieee80211_key *wk,
    const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct uwe5622_sdio_softc *sc = ic->ic_ifp->if_softc;
	uint8_t buf[1 + sizeof(struct uwe_sprdwl_cmd_add_key) +
	    IEEE80211_KEYBUF_SIZE + IEEE80211_MICBUF_SIZE];
	struct uwe_sprdwl_cmd_add_key *key;
	uint8_t reply[UWE_WIFI_MAX_REPLY], rctx, cipher;
	size_t reply_len, key_len, paylen;
	int8_t status;
	int err;

	if (!sc->sc_wifi_session_ready)
		return 0;

	switch (wk->wk_cipher->ic_cipher) {
	case IEEE80211_CIPHER_WEP:
		cipher = wk->wk_keylen == 5 ? UWE_SPRDWL_CIPHER_WEP40 :
		    UWE_SPRDWL_CIPHER_WEP104;
		break;
	case IEEE80211_CIPHER_TKIP:
		cipher = UWE_SPRDWL_CIPHER_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		cipher = UWE_SPRDWL_CIPHER_CCMP;
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "KEY add: unsupported cipher=%u\n",
		    wk->wk_cipher->ic_cipher);
		return 0;
	}

	key_len = wk->wk_keylen;
	if (key_len > IEEE80211_KEYBUF_SIZE + IEEE80211_MICBUF_SIZE)
		return 0;
	memset(buf, 0, sizeof(buf));
	buf[0] = UWE_SPRDWL_KEY_ADD;
	key = (struct uwe_sprdwl_cmd_add_key *)(buf + 1);
	key->key_index = (uint8_t)wk->wk_keyix;
	key->pairwise = (wk->wk_flags & IEEE80211_KEY_GROUP) == 0;
	/* Firmware identifies a station pairwise key by the AP/LUT peer. */
	if (key->pairwise && sc->sc_sta_lut_valid)
		memcpy(key->mac, sc->sc_sta_ra, UWE_WIFI_MAC_LEN);
	else
		memcpy(key->mac, mac, UWE_WIFI_MAC_LEN);
	le64enc(key->keyseq, wk->wk_keyrsc);
	key->cipher_type = cipher;
	key->key_len = (uint8_t)key_len;
	memcpy(key->value, wk->wk_key, key_len);
	paylen = 1 + sizeof(*key) + key_len;

	reply_len = sizeof(reply);
	err = uwe5622_sdio_send_wifi_cmd(sc, UWE_WIFI_CMD_KEY,
	    sc->sc_wifi_ctx_id, buf, paylen, true);
	if (err == 0)
		err = uwe5622_sdio_wait_wifi_reply(sc, UWE_WIFI_CMD_KEY,
		    UWE_WIFI_CMD_TIMEOUT_MS, reply, &reply_len, &rctx, &status);
	if (err != 0 || status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "KEY add failed index=%u pairwise=%u cipher=%u len=%zu "
		    "flags=0x%x mac=%s err=%d status=%d\n",
		    key->key_index, key->pairwise, cipher, key_len,
		    wk->wk_flags, ether_sprintf(key->mac), err,
		    err == 0 ? status : -1);
		return 0;
	}

	aprint_normal_dev(sc->sc_dev,
	    "KEY add ok index=%u pairwise=%u cipher=%u len=%zu mac=%s\n",
	    key->key_index, key->pairwise, cipher, key_len,
	    ether_sprintf(mac));
	return 1;
}

int
uwe5622_sdio_key_delete(struct ieee80211com *ic,
    const struct ieee80211_key *wk)
{
	/*
	 * net80211 calls cs_key_delete while holding its crypto spin mutex.
	 * SDIO command execution sleeps, so issuing firmware KEY DEL here
	 * panics in mi_switch.  Firmware removes per-peer keys when the LUT is
	 * deleted/disconnected; defer a complete asynchronous DEL implementation.
	 */
	(void)ic;
	(void)wk;
	return 1;
}

void
uwe5622_sdio_net80211_sync_mac(struct uwe5622_sdio_softc *sc)
{
	struct ieee80211com *ic;
	struct ifnet *ifp;

	if (!sc->sc_net80211_attached)
		return;
	if (!(sc->sc_fw_mac[0] || sc->sc_fw_mac[1] || sc->sc_fw_mac[2] ||
	    sc->sc_fw_mac[3] || sc->sc_fw_mac[4] || sc->sc_fw_mac[5]))
		return;

	ic = &sc->sc_ic;
	ifp = ic->ic_ifp;
	memcpy(ic->ic_myaddr, sc->sc_fw_mac, sizeof(sc->sc_fw_mac));
	if (ifp != NULL)
		if_set_sadl(ifp, sc->sc_fw_mac, sizeof(sc->sc_fw_mac), false);

	aprint_normal_dev(sc->sc_dev,
	    "net80211 mac sync: %s\n", ether_sprintf(ic->ic_myaddr));
}

void
uwe5622_sdio_net80211_setup_channels(struct uwe5622_sdio_softc *sc)
{
	struct ieee80211com *ic;
	size_t j;
	int i;

	ic = &sc->sc_ic;
	for (i = 0; i < IEEE80211_CHAN_MAX + 1; i++) {
		ic->ic_channels[i].ic_freq = 0;
		ic->ic_channels[i].ic_flags = 0;
	}

	for (i = 1; i <= 11; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
		    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
	}

	for (j = 0; j < __arraycount(uwe_5ghz_channels); j++) {
		const uint8_t chan = uwe_5ghz_channels[j];
		ic->ic_channels[chan].ic_freq =
		    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_5GHZ);
		ic->ic_channels[chan].ic_flags =
		    IEEE80211_CHAN_A | IEEE80211_CHAN_5GHZ;
	}

	ic->ic_ibss_chan = &ic->ic_channels[1];
	ic->ic_des_chan = IEEE80211_CHAN_ANYC;

	aprint_normal_dev(sc->sc_dev,
	    "net80211 channels: 2.4GHz=1-11 5GHz=%zu channels enabled\n",
	    __arraycount(uwe_5ghz_channels));
}

void
uwe5622_sdio_net80211_attach(struct uwe5622_sdio_softc *sc)
{
	struct ieee80211com *ic;
	struct ifnet *ifp;
	uint8_t lladdr[UWE_WIFI_MAC_LEN];

	ic = &sc->sc_ic;
	ifp = &sc->sc_if;

	memset(ifp, 0, sizeof(*ifp));
	memset(ic, 0, sizeof(*ic));
	memset(lladdr, 0, sizeof(lladdr));

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_caps = IEEE80211_C_WPA2;
	memset(&ic->ic_stats, 0, sizeof(ic->ic_stats));

	if (sc->sc_fw_mac[0] || sc->sc_fw_mac[1] || sc->sc_fw_mac[2] ||
	    sc->sc_fw_mac[3] || sc->sc_fw_mac[4] || sc->sc_fw_mac[5]) {
		memcpy(lladdr, sc->sc_fw_mac, sizeof(lladdr));
	} else {
		lladdr[0] = 0x02;
		lladdr[1] = 0x55;
		lladdr[2] = 0x22;
		lladdr[3] = 0x33;
		lladdr[4] = 0x44;
		lladdr[5] = 0x66;
	}
	memcpy(ic->ic_myaddr, lladdr, sizeof(lladdr));

	uwe5622_sdio_net80211_setup_channels(sc);

	if_initialize(ifp);
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = uwe5622_sdio_if_init;
	ifp->if_stop = uwe5622_sdio_if_stop;
	ifp->if_ioctl = uwe5622_sdio_if_ioctl;
	ifp->if_start = uwe5622_sdio_if_start;
	ifp->if_watchdog = uwe5622_sdio_if_watchdog;

	ieee80211_ifattach(ic);
	ic->ic_crypto.cs_key_set = uwe5622_sdio_key_set;
	ic->ic_crypto.cs_key_delete = uwe5622_sdio_key_delete;
	ic->ic_crypto.cs_max_keyix = IEEE80211_WEP_NKID;
	/*
	 * Default roaming mode is AUTO, under which ieee80211_end_scan()
	 * itself picks a candidate matching ic_des_essid and calls
	 * ieee80211_sta_join() - consuming/altering ic_scan's node table as
	 * a side effect, racing with our own manual (sysctl-triggered)
	 * SSID/channel/BSSID lookup in uwe5622_sdio_wifi_cmd_connect(). We
	 * drive association ourselves entirely via firmware commands, so
	 * tell net80211 to keep its hands off.
	 */
	ic->ic_roaming = IEEE80211_ROAMING_MANUAL;
	memset(&ic->ic_stats, 0, sizeof(ic->ic_stats));
	ieee80211_media_init(ic, uwe5622_sdio_media_change,
	    uwe5622_sdio_media_status);
	/* Use the common softint-backed input path for firmware RX data. */
	ifp->if_percpuq = if_percpuq_create(ifp);
	if_register(ifp);

	if (workqueue_create(&sc->sc_tx_wq, "uwe5622tx",
	    uwe5622_sdio_tx_work, sc, PRI_SOFTNET, IPL_NET, WQ_MPSAFE) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "TX workqueue creation failed, TX will be disabled\n");
		sc->sc_tx_wq = NULL;
	}

	sc->sc_rx_stop = false;
	if (kthread_create(PRI_SOFTNET, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN,
	    NULL, uwe5622_sdio_rx_thread, sc, &sc->sc_rx_lwp,
	    "uwe5622rx") != 0) {
		aprint_error_dev(sc->sc_dev,
		    "RX poll thread creation failed, RX will be disabled\n");
		sc->sc_rx_lwp = NULL;
	}

	sc->sc_net80211_attached = true;
	uwe5622_sdio_net80211_sync_mac(sc);
	aprint_normal_dev(sc->sc_dev,
	    "net80211 attach complete: interface %s mac=%s watchdog=yes\n",
	    ifp->if_xname, ether_sprintf(ic->ic_myaddr));
}

void
uwe5622_sdio_net80211_detach(struct uwe5622_sdio_softc *sc)
{
	if (!sc->sc_net80211_attached)
		return;

	uwe5622_sdio_if_stop(sc->sc_ic.ic_ifp, 1);
	ieee80211_ifdetach(&sc->sc_ic);
	sc->sc_net80211_attached = false;

	if (sc->sc_rx_lwp != NULL) {
		sc->sc_rx_stop = true;
		kthread_join(sc->sc_rx_lwp);
		sc->sc_rx_lwp = NULL;
	}

	if (sc->sc_tx_wq != NULL) {
		workqueue_wait(sc->sc_tx_wq, &sc->sc_tx_work);
		workqueue_destroy(sc->sc_tx_wq);
		sc->sc_tx_wq = NULL;
	}
}

int
uwe5622_sdio_sysctl_psk(SYSCTLFN_ARGS)
{
	char buf[UWE_WIFI_PSK_CFG_LEN + 1];
	char ssid[IEEE80211_NWID_LEN + 1];
	struct uwe5622_sdio_softc *sc;
	struct sysctlnode node;
	struct ieee80211com *ic;
	int error;
	int esslen;

	node = *rnode;
	sc = node.sysctl_data;
	memcpy(buf, sc->sc_wifi_psk_cfg, sizeof(buf));
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	strlcpy(sc->sc_wifi_psk_cfg, buf, sizeof(sc->sc_wifi_psk_cfg));

	ic = &sc->sc_ic;
	esslen = ic->ic_des_esslen;
	if (esslen <= 0) {
		aprint_error_dev(sc->sc_dev,
		    "wifi_psk set but no SSID selected - "
		    "run 'ifconfig %s nwid <ssid>' first\n",
		    device_xname(sc->sc_dev));
		return 0;
	}
	if (esslen > IEEE80211_NWID_LEN)
		esslen = IEEE80211_NWID_LEN;
	memcpy(ssid, ic->ic_des_essid, esslen);
	ssid[esslen] = '\0';

	if (!sc->sc_wifi_session_ready) {
		error = uwe5622_sdio_ensure_wifi_session(sc);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "wifi session setup failed err=%d\n", error);
			return 0;
		}
	}

	aprint_normal_dev(sc->sc_dev,
	    "wifi_psk set, triggering connect to ssid='%s'\n", ssid);
	(void)uwe5622_sdio_wifi_cmd_connect(sc, sc->sc_wifi_ctx_id, NULL, 0,
	    ssid, sc->sc_wifi_psk_cfg);
	return 0;
}

/*
 * Write-triggered: prints the RX type/subtype histogram
 * (uwe5622_sdio_pkt_stat_dump(), previously dead/__unused code) - added to
 * directly answer "what's actually inside the recurring valid_len=1040/1424
 * aggregates" (subtype=12 real DATA vs firmware log spam vs something else)
 * without guessing from raw hex.
 */
int
uwe5622_sdio_sysctl_dump_stats(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct uwe5622_sdio_softc *sc;
	bool val;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	val = false;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (val) {
		struct ifnet *ifp = &sc->sc_if;

		uwe5622_sdio_pkt_stat_dump(sc);
		aprint_normal_dev(sc->sc_dev,
		    "TX census: if_start=%u claims=%u busy=%u workers=%u "
		    "dequeued=%u ok=%u error=%u no_credit=%u no_lut=%u\n",
		    sc->sc_tx_start_calls, sc->sc_tx_schedule_claims,
		    sc->sc_tx_schedule_busy, sc->sc_tx_worker_runs,
		    sc->sc_tx_dequeued, sc->sc_tx_ok, sc->sc_tx_error,
		    sc->sc_tx_no_credit, sc->sc_tx_no_lut);
		aprint_normal_dev(sc->sc_dev,
		    "TX state: qlen=%d pending=%u rechecks=%u race_suspect=%u "
		    "credit_waits=%u waiting=%u credits=%u/%u/%u/%u "
		    "used=%u/%u/%u/%u lut_valid=%u lut=%u fw_assert=%u\n",
		    ifp->if_snd.ifq_len, sc->sc_tx_pending,
		    sc->sc_tx_rechecks, sc->sc_tx_lost_wakeup_suspect,
		    sc->sc_tx_credit_waits, sc->sc_tx_waiting_credit,
		    sc->sc_tx_credit[0], sc->sc_tx_credit[1],
		    sc->sc_tx_credit[2], sc->sc_tx_credit[3],
		    sc->sc_tx_color_used[0], sc->sc_tx_color_used[1],
		    sc->sc_tx_color_used[2], sc->sc_tx_color_used[3],
		    sc->sc_sta_lut_valid, sc->sc_sta_lut_index,
		    sc->sc_fw_tx_asserted);
		aprint_normal_dev(sc->sc_dev,
		    "protocol census: BA=%u ba_rsp=%u/%u ip_notify=%u/%u/%u "
		    "ipv4=%u.%u.%u.%u\n",
		    sc->sc_ba_events, sc->sc_ba_rsp_ok, sc->sc_ba_rsp_error,
		    sc->sc_ip_notify_attempts,
		    sc->sc_ip_notify_ok, sc->sc_ip_notify_error,
		    sc->sc_notified_ipv4[0], sc->sc_notified_ipv4[1],
		    sc->sc_notified_ipv4[2], sc->sc_notified_ipv4[3]);
	}
	return 0;
}

void
uwe5622_sdio_sysctl_attach(struct uwe5622_sdio_softc *sc)
{
	const struct sysctlnode *rnode, *cnode;
	int error;

	sc->sc_sysctllog = NULL;

	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("uwe5622_sdio controls"), NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "sysctl node creation failed err=%d\n", error);
		return;
	}

	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "wifi_psk",
	    SYSCTL_DESCR("WPA2 passphrase for the ifconfig-selected nwid; "
		"writing this triggers connect"),
	    uwe5622_sdio_sysctl_psk, 0, (void *)sc,
	    UWE_WIFI_PSK_CFG_LEN + 1,
	    CTL_HW, rnode->sysctl_num, CTL_CREATE, CTL_EOL);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "sysctl wifi_psk creation failed err=%d\n", error);

	sc->sc_verbose = false;
	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "verbose",
	    SYSCTL_DESCR("log every TX send / RX poll hit at normal level "
		"(noisy!) - set to 0 to quiet the console at runtime"),
	    NULL, 0, &sc->sc_verbose, 0,
	    CTL_HW, rnode->sysctl_num, CTL_CREATE, CTL_EOL);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "sysctl verbose creation failed err=%d\n", error);

	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "dump_stats",
	    SYSCTL_DESCR("write 1 to print the RX type/subtype packet "
		"histogram (what's really inside the aggregates) at "
		"normal log level"),
	    uwe5622_sdio_sysctl_dump_stats, 0, (void *)sc, 0,
	    CTL_HW, rnode->sysctl_num, CTL_CREATE, CTL_EOL);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "sysctl dump_stats creation failed err=%d\n", error);
}

int
uwe5622_sdio_ensure_wifi_session(struct uwe5622_sdio_softc *sc)
{
	uint8_t ctx;
	int err;

	if (sc->sc_wifi_session_ready)
		return 0;

	if (!sc->sc_wifi_opened) {
		err = uwe5622_sdio_wifi_cmd_open(sc, 1, NULL, &ctx);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "bridge: OPEN failed err=%d\n", err);
			return err;
		}
		sc->sc_wifi_opened = true;
	}

	err = uwe5622_sdio_wifi_cmd_get_info(sc, sc->sc_wifi_ctx_id);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: GET_INFO failed err=%d\n", err);
		return err;
	}

	/*
	 * OPEN requires the station MAC in its payload.  At initial attach we
	 * do not know the firmware's permanent MAC yet, so the bootstrap OPEN
	 * above used an all-zero address.  The firmware responds by creating a
	 * VIF with a random locally administered MAC, while GET_INFO then makes
	 * net80211 use the permanent MAC.  That split identity lets management
	 * frames reach the air but causes Ethernet data from net80211 to be
	 * rejected by the firmware VIF.
	 *
	 * Now that GET_INFO supplied sc_fw_mac, discard the bootstrap VIF and
	 * reopen it with the same MAC exposed by net80211.  Do this before INI,
	 * regulatory, scan, and connect commands so every subsequent operation
	 * uses the final context.
	 */
	err = uwe5622_sdio_wifi_cmd_close(sc, sc->sc_wifi_ctx_id, 1);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: bootstrap CLOSE failed err=%d\n", err);
		return err;
	}
	sc->sc_wifi_opened = false;

	err = uwe5622_sdio_wifi_cmd_open(sc, 1, sc->sc_fw_mac, &ctx);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: MAC-bound OPEN failed err=%d\n", err);
		return err;
	}
	sc->sc_wifi_opened = true;
	aprint_normal_dev(sc->sc_dev,
	    "bridge: reopened STA with firmware MAC "
	    "%02x:%02x:%02x:%02x:%02x:%02x ctx=%u\n",
	    sc->sc_fw_mac[0], sc->sc_fw_mac[1], sc->sc_fw_mac[2],
	    sc->sc_fw_mac[3], sc->sc_fw_mac[4], sc->sc_fw_mac[5], ctx);

	err = uwe5622_sdio_wifi_download_ini(sc, sc->sc_wifi_ctx_id);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: DOWNLOAD_INI failed err=%d\n", err);
		return err;
	}

	/*
	 * DISABLED AGAIN: was re-enabled earlier as an experiment to see if
	 * an explicit regulatory hint would unlock real TX (theory: firmware
	 * defaults to passive/RX-only until a regdomain is set). That theory
	 * is now moot - the real TX blockers turned out to be the wire
	 * format (OTT prefix/subtype), the credit/flow-control system, and
	 * the missing WPA2 SET_IE/KEY plumbing, all fixed since. SET_REGDOM
	 * itself was never shown to provide any actual benefit (2.4GHz
	 * TX/RX/WPA2/throughput all work perfectly without it), while
	 * directly confirmed (2026-08-26, real hardware: a known 5GHz AP,
	 * "sincap5", never once appeared in ANY scan result - neither
	 * wpa_supplicant's own log nor the kernel driver's own
	 * "mgmt parsed: ... ssid=" scan log - while sending this call every
	 * session) to still cause the exact 5GHz-scan-breaking regression
	 * first found much earlier in this project. Left the function
	 * itself defined below in case the rule-table format is ever worth
	 * revisiting, but do not call it unconditionally on every bring-up.
	 */
#if 0
	err = uwe5622_sdio_wifi_set_regdom_world(sc, sc->sc_wifi_ctx_id);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: SET_REGDOM failed err=%d (continuing)\n", err);
	}
#endif

	sc->sc_wifi_session_ready = true;
	aprint_normal_dev(sc->sc_dev,
	    "bridge: wifi session ready ctx=%u\n", sc->sc_wifi_ctx_id);
	return 0;
}

int
uwe5622_sdio_bridge_scan_request(struct uwe5622_sdio_softc *sc)
{
	int err;

	err = uwe5622_sdio_ensure_wifi_session(sc);
	if (err)
		return err;

	/*
	 * Firmware scans abort an in-progress association/data context.  The
	 * BSD supplicant may ask for another scan immediately after EVENT_ASSOC,
	 * while the four-way handshake is still exchanging EAPOL frames.  On
	 * this full-MAC device that produced a STA_LUT delete and DISCONNECT in
	 * the middle of the handshake.  Roaming is not implemented yet, so keep
	 * the established firmware peer intact and satisfy the request from the
	 * existing net80211 scan cache.
	 */
	if (sc->sc_sta_lut_valid) {
		if (sc->sc_verbose)
			aprint_normal_dev(sc->sc_dev,
			    "bridge: suppressing scan while associated (LUT=%u)\n",
			    sc->sc_sta_lut_index);
		uwe5622_sdio_finish_net80211_scan(sc);
		return 0;
	}

 	uwe5622_sdio_begin_net80211_scan(sc);
	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "bridge: starting firmware scan ctx=%u mask=0x%08x\n",
		    sc->sc_wifi_ctx_id, 0x000007ffU);
	err = uwe5622_sdio_wifi_cmd_scan(sc, sc->sc_wifi_ctx_id, 0x000007ffU);
 	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "bridge: firmware scan failed err=%d\n", err);
		uwe5622_sdio_finish_net80211_scan(sc);
		return err;
	}

	if (sc->sc_verbose)
		aprint_normal_dev(sc->sc_dev,
		    "bridge: firmware scan finished\n");
	return 0;
}

const char *
uwe5622_sdio_ioctl_name(u_long cmd)
{
	switch (cmd) {
#ifdef SIOCS80211NWID
	case SIOCS80211NWID:
		return "SIOCS80211NWID";
#endif
#ifdef SIOCG80211NWID
	case SIOCG80211NWID:
		return "SIOCG80211NWID";
#endif
#ifdef SIOCS80211SCAN
	case SIOCS80211SCAN:
		return "SIOCS80211SCAN";
#endif
#ifdef SIOCG80211ALLCHANS
	case SIOCG80211ALLCHANS:
		return "SIOCG80211ALLCHANS";
#endif
#ifdef SIOCG80211BSSID
	case SIOCG80211BSSID:
		return "SIOCG80211BSSID";
#endif
#ifdef SIOCG80211CHANNEL
	case SIOCG80211CHANNEL:
		return "SIOCG80211CHANNEL";
#endif
#ifdef SIOCG80211STATS
	case SIOCG80211STATS:
		return "SIOCG80211STATS";
#endif
	case 0x802069f4:
		return "SCAN_TRIGGER_RAW";
	case 0xc0906911:
		return "SCAN_PREP_RAW";
#ifdef SIOCG80211ZSTATS
	case SIOCG80211ZSTATS:
		return "SIOCG80211ZSTATS";
#endif
	default:
		return "unknown";
	}
}
