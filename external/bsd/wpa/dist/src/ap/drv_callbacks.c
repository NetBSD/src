/*
 * hostapd / Callback functions for driver wrappers
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "radius/radius.h"
#include "drivers/driver.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/wpa_ctrl.h"
#include "crypto/random.h"
#include "p2p/p2p.h"
#include "wps/wps.h"
#include "wnm_ap.h"
#include "hostapd.h"
#include "ieee802_11.h"
#include "sta_info.h"
#include "accounting.h"
#include "tkip_countermeasures.h"
#include "ieee802_1x.h"
#include "wpa_auth.h"
#include "wps_hostapd.h"
#include "ap_drv_ops.h"
#include "ap_config.h"
#include "hw_features.h"
#include "dfs.h"
#include "beacon.h"


int hostapd_notif_assoc(struct hostapd_data *hapd, const u8 *addr,
			const u8 *req_ies, size_t req_ies_len, int reassoc)
{
	struct sta_info *sta;
	int new_assoc, res;
	struct ieee802_11_elems elems;
	const u8 *ie;
	size_t ielen;
#ifdef CONFIG_IEEE80211R
	u8 buf[sizeof(struct ieee80211_mgmt) + 1024];
	u8 *p = buf;
#endif /* CONFIG_IEEE80211R */
	u16 reason = WLAN_REASON_UNSPECIFIED;
	u16 status = WLAN_STATUS_SUCCESS;
	const u8 *p2p_dev_addr = NULL;

	if (addr == NULL) {
		/*
		 * This could potentially happen with unexpected event from the
		 * driver wrapper. This was seen at least in one case where the
		 * driver ended up being set to station mode while hostapd was
		 * running, so better make sure we stop processing such an
		 * event here.
		 */
		wpa_printf(MSG_DEBUG, "hostapd_notif_assoc: Skip event with "
			   "no address");
		return -1;
	}
	random_add_randomness(addr, ETH_ALEN);

	hostapd_logger(hapd, addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO, "associated");

	ieee802_11_parse_elems(req_ies, req_ies_len, &elems, 0);
	if (elems.wps_ie) {
		ie = elems.wps_ie - 2;
		ielen = elems.wps_ie_len + 2;
		wpa_printf(MSG_DEBUG, "STA included WPS IE in (Re)AssocReq");
	} else if (elems.rsn_ie) {
		ie = elems.rsn_ie - 2;
		ielen = elems.rsn_ie_len + 2;
		wpa_printf(MSG_DEBUG, "STA included RSN IE in (Re)AssocReq");
	} else if (elems.wpa_ie) {
		ie = elems.wpa_ie - 2;
		ielen = elems.wpa_ie_len + 2;
		wpa_printf(MSG_DEBUG, "STA included WPA IE in (Re)AssocReq");
#ifdef CONFIG_HS20
	} else if (elems.osen) {
		ie = elems.osen - 2;
		ielen = elems.osen_len + 2;
		wpa_printf(MSG_DEBUG, "STA included OSEN IE in (Re)AssocReq");
#endif /* CONFIG_HS20 */
	} else {
		ie = NULL;
		ielen = 0;
		wpa_printf(MSG_DEBUG, "STA did not include WPS/RSN/WPA IE in "
			   "(Re)AssocReq");
	}

	sta = ap_get_sta(hapd, addr);
	if (sta) {
		ap_sta_no_session_timeout(hapd, sta);
		accounting_sta_stop(hapd, sta);

		/*
		 * Make sure that the previously registered inactivity timer
		 * will not remove the STA immediately.
		 */
		sta->timeout_next = STA_NULLFUNC;
	} else {
		sta = ap_sta_add(hapd, addr);
		if (sta == NULL) {
			hostapd_drv_sta_disassoc(hapd, addr,
						 WLAN_REASON_DISASSOC_AP_BUSY);
			return -1;
		}
	}
	sta->flags &= ~(WLAN_STA_WPS | WLAN_STA_MAYBE_WPS | WLAN_STA_WPS2);

#ifdef CONFIG_P2P
	if (elems.p2p) {
		wpabuf_free(sta->p2p_ie);
		sta->p2p_ie = ieee802_11_vendor_ie_concat(req_ies, req_ies_len,
							  P2P_IE_VENDOR_TYPE);
		if (sta->p2p_ie)
			p2p_dev_addr = p2p_get_go_dev_addr(sta->p2p_ie);
	}
#endif /* CONFIG_P2P */

#ifdef CONFIG_IEEE80211N
#ifdef NEED_AP_MLME
	if (elems.ht_capabilities &&
	    elems.ht_capabilities_len >=
	    sizeof(struct ieee80211_ht_capabilities) &&
	    (hapd->iface->conf->ht_capab &
	     HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET)) {
		struct ieee80211_ht_capabilities *ht_cap =
			(struct ieee80211_ht_capabilities *)
			elems.ht_capabilities;

		if (le_to_host16(ht_cap->ht_capabilities_info) &
		    HT_CAP_INFO_40MHZ_INTOLERANT)
			ht40_intolerant_add(hapd->iface, sta);
	}
#endif /* NEED_AP_MLME */
#endif /* CONFIG_IEEE80211N */

#ifdef CONFIG_INTERWORKING
	if (elems.ext_capab && elems.ext_capab_len > 4) {
		if (elems.ext_capab[4] & 0x01)
			sta->qos_map_enabled = 1;
	}
#endif /* CONFIG_INTERWORKING */

#ifdef CONFIG_HS20
	wpabuf_free(sta->hs20_ie);
	if (elems.hs20 && elems.hs20_len > 4) {
		sta->hs20_ie = wpabuf_alloc_copy(elems.hs20 + 4,
						 elems.hs20_len - 4);
	} else
		sta->hs20_ie = NULL;
#endif /* CONFIG_HS20 */

	if (hapd->conf->wpa) {
		if (ie == NULL || ielen == 0) {
#ifdef CONFIG_WPS
			if (hapd->conf->wps_state) {
				wpa_printf(MSG_DEBUG, "STA did not include "
					   "WPA/RSN IE in (Re)Association "
					   "Request - possible WPS use");
				sta->flags |= WLAN_STA_MAYBE_WPS;
				goto skip_wpa_check;
			}
#endif /* CONFIG_WPS */

			wpa_printf(MSG_DEBUG, "No WPA/RSN IE from STA");
			return -1;
		}
#ifdef CONFIG_WPS
		if (hapd->conf->wps_state && ie[0] == 0xdd && ie[1] >= 4 &&
		    os_memcmp(ie + 2, "\x00\x50\xf2\x04", 4) == 0) {
			struct wpabuf *wps;
			sta->flags |= WLAN_STA_WPS;
			wps = ieee802_11_vendor_ie_concat(ie, ielen,
							  WPS_IE_VENDOR_TYPE);
			if (wps) {
				if (wps_is_20(wps)) {
					wpa_printf(MSG_DEBUG, "WPS: STA "
						   "supports WPS 2.0");
					sta->flags |= WLAN_STA_WPS2;
				}
				wpabuf_free(wps);
			}
			goto skip_wpa_check;
		}
#endif /* CONFIG_WPS */

		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr,
							p2p_dev_addr);
		if (sta->wpa_sm == NULL) {
			wpa_printf(MSG_ERROR, "Failed to initialize WPA state "
				   "machine");
			return -1;
		}
		res = wpa_validate_wpa_ie(hapd->wpa_auth, sta->wpa_sm,
					  ie, ielen,
					  elems.mdie, elems.mdie_len);
		if (res != WPA_IE_OK) {
			wpa_printf(MSG_DEBUG, "WPA/RSN information element "
				   "rejected? (res %u)", res);
			wpa_hexdump(MSG_DEBUG, "IE", ie, ielen);
			if (res == WPA_INVALID_GROUP) {
				reason = WLAN_REASON_GROUP_CIPHER_NOT_VALID;
				status = WLAN_STATUS_GROUP_CIPHER_NOT_VALID;
			} else if (res == WPA_INVALID_PAIRWISE) {
				reason = WLAN_REASON_PAIRWISE_CIPHER_NOT_VALID;
				status = WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID;
			} else if (res == WPA_INVALID_AKMP) {
				reason = WLAN_REASON_AKMP_NOT_VALID;
				status = WLAN_STATUS_AKMP_NOT_VALID;
			}
#ifdef CONFIG_IEEE80211W
			else if (res == WPA_MGMT_FRAME_PROTECTION_VIOLATION) {
				reason = WLAN_REASON_INVALID_IE;
				status = WLAN_STATUS_INVALID_IE;
			} else if (res == WPA_INVALID_MGMT_GROUP_CIPHER) {
				reason = WLAN_REASON_GROUP_CIPHER_NOT_VALID;
				status = WLAN_STATUS_GROUP_CIPHER_NOT_VALID;
			}
#endif /* CONFIG_IEEE80211W */
			else {
				reason = WLAN_REASON_INVALID_IE;
				status = WLAN_STATUS_INVALID_IE;
			}
			goto fail;
		}
#ifdef CONFIG_IEEE80211W
		if ((sta->flags & WLAN_STA_MFP) && !sta->sa_query_timed_out &&
		    sta->sa_query_count > 0)
			ap_check_sa_query_timeout(hapd, sta);
		if ((sta->flags & WLAN_STA_MFP) && !sta->sa_query_timed_out &&
		    (sta->auth_alg != WLAN_AUTH_FT)) {
			/*
			 * STA has already been associated with MFP and SA
			 * Query timeout has not been reached. Reject the
			 * association attempt temporarily and start SA Query,
			 * if one is not pending.
			 */

			if (sta->sa_query_count == 0)
				ap_sta_start_sa_query(hapd, sta);

#ifdef CONFIG_IEEE80211R
			status = WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY;

			p = hostapd_eid_assoc_comeback_time(hapd, sta, p);

			hostapd_sta_assoc(hapd, addr, reassoc, status, buf,
					  p - buf);
#endif /* CONFIG_IEEE80211R */
			return 0;
		}

		if (wpa_auth_uses_mfp(sta->wpa_sm))
			sta->flags |= WLAN_STA_MFP;
		else
			sta->flags &= ~WLAN_STA_MFP;
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_IEEE80211R
		if (sta->auth_alg == WLAN_AUTH_FT) {
			status = wpa_ft_validate_reassoc(sta->wpa_sm, req_ies,
							 req_ies_len);
			if (status != WLAN_STATUS_SUCCESS) {
				if (status == WLAN_STATUS_INVALID_PMKID)
					reason = WLAN_REASON_INVALID_IE;
				if (status == WLAN_STATUS_INVALID_MDIE)
					reason = WLAN_REASON_INVALID_IE;
				if (status == WLAN_STATUS_INVALID_FTIE)
					reason = WLAN_REASON_INVALID_IE;
				goto fail;
			}
		}
#endif /* CONFIG_IEEE80211R */
	} else if (hapd->conf->wps_state) {
#ifdef CONFIG_WPS
		struct wpabuf *wps;
		if (req_ies)
			wps = ieee802_11_vendor_ie_concat(req_ies, req_ies_len,
							  WPS_IE_VENDOR_TYPE);
		else
			wps = NULL;
#ifdef CONFIG_WPS_STRICT
		if (wps && wps_validate_assoc_req(wps) < 0) {
			reason = WLAN_REASON_INVALID_IE;
			status = WLAN_STATUS_INVALID_IE;
			wpabuf_free(wps);
			goto fail;
		}
#endif /* CONFIG_WPS_STRICT */
		if (wps) {
			sta->flags |= WLAN_STA_WPS;
			if (wps_is_20(wps)) {
				wpa_printf(MSG_DEBUG, "WPS: STA supports "
					   "WPS 2.0");
				sta->flags |= WLAN_STA_WPS2;
			}
		} else
			sta->flags |= WLAN_STA_MAYBE_WPS;
		wpabuf_free(wps);
#endif /* CONFIG_WPS */
#ifdef CONFIG_HS20
	} else if (hapd->conf->osen) {
		if (elems.osen == NULL) {
			hostapd_logger(
				hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
				HOSTAPD_LEVEL_INFO,
				"No HS 2.0 OSEN element in association request");
			return WLAN_STATUS_INVALID_IE;
		}

		wpa_printf(MSG_DEBUG, "HS 2.0: OSEN association");
		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr, NULL);
		if (sta->wpa_sm == NULL) {
			wpa_printf(MSG_WARNING, "Failed to initialize WPA "
				   "state machine");
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
		}
		if (wpa_validate_osen(hapd->wpa_auth, sta->wpa_sm,
				      elems.osen - 2, elems.osen_len + 2) < 0)
			return WLAN_STATUS_INVALID_IE;
#endif /* CONFIG_HS20 */
	}
#ifdef CONFIG_WPS
skip_wpa_check:
#endif /* CONFIG_WPS */

#ifdef CONFIG_IEEE80211R
	p = wpa_sm_write_assoc_resp_ies(sta->wpa_sm, buf, sizeof(buf),
					sta->auth_alg, req_ies, req_ies_len);

	hostapd_sta_assoc(hapd, addr, reassoc, status, buf, p - buf);

	if (sta->auth_alg == WLAN_AUTH_FT)
		ap_sta_set_authorized(hapd, sta, 1);
#else /* CONFIG_IEEE80211R */
	/* Keep compiler silent about unused variables */
	if (status) {
	}
#endif /* CONFIG_IEEE80211R */

	new_assoc = (sta->flags & WLAN_STA_ASSOC) == 0;
	sta->flags |= WLAN_STA_AUTH | WLAN_STA_ASSOC;
	sta->flags &= ~WLAN_STA_WNM_SLEEP_MODE;

	hostapd_set_sta_flags(hapd, sta);

	if (reassoc && (sta->auth_alg == WLAN_AUTH_FT))
		wpa_auth_sm_event(sta->wpa_sm, WPA_ASSOC_FT);
	else
		wpa_auth_sm_event(sta->wpa_sm, WPA_ASSOC);

	hostapd_new_assoc_sta(hapd, sta, !new_assoc);

	ieee802_1x_notify_port_enabled(sta->eapol_sm, 1);

#ifdef CONFIG_P2P
	if (req_ies) {
		p2p_group_notif_assoc(hapd->p2p_group, sta->addr,
				      req_ies, req_ies_len);
	}
#endif /* CONFIG_P2P */

	return 0;

fail:
#ifdef CONFIG_IEEE80211R
	hostapd_sta_assoc(hapd, addr, reassoc, status, buf, p - buf);
#endif /* CONFIG_IEEE80211R */
	hostapd_drv_sta_disassoc(hapd, sta->addr, reason);
	ap_free_sta(hapd, sta);
	return -1;
}


void hostapd_notif_disassoc(struct hostapd_data *hapd, const u8 *addr)
{
	struct sta_info *sta;

	if (addr == NULL) {
		/*
		 * This could potentially happen with unexpected event from the
		 * driver wrapper. This was seen at least in one case where the
		 * driver ended up reporting a station mode event while hostapd
		 * was running, so better make sure we stop processing such an
		 * event here.
		 */
		wpa_printf(MSG_DEBUG, "hostapd_notif_disassoc: Skip event "
			   "with no address");
		return;
	}

	hostapd_logger(hapd, addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO, "disassociated");

	sta = ap_get_sta(hapd, addr);
	if (sta == NULL) {
		wpa_printf(MSG_DEBUG, "Disassociation notification for "
			   "unknown STA " MACSTR, MAC2STR(addr));
		return;
	}

	ap_sta_set_authorized(hapd, sta, 0);
	sta->flags &= ~(WLAN_STA_AUTH | WLAN_STA_ASSOC);
	wpa_auth_sm_event(sta->wpa_sm, WPA_DISASSOC);
	sta->acct_terminate_cause = RADIUS_ACCT_TERMINATE_CAUSE_USER_REQUEST;
	ieee802_1x_notify_port_enabled(sta->eapol_sm, 0);
	ap_free_sta(hapd, sta);
}


void hostapd_event_sta_low_ack(struct hostapd_data *hapd, const u8 *addr)
{
	struct sta_info *sta = ap_get_sta(hapd, addr);

	if (!sta || !hapd->conf->disassoc_low_ack)
		return;

	hostapd_logger(hapd, addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO, "disconnected due to excessive "
		       "missing ACKs");
	hostapd_drv_sta_disassoc(hapd, addr, WLAN_REASON_DISASSOC_LOW_ACK);
	if (sta)
		ap_sta_disassociate(hapd, sta, WLAN_REASON_DISASSOC_LOW_ACK);
}


void hostapd_event_ch_switch(struct hostapd_data *hapd, int freq, int ht,
			     int offset, int width, int cf1, int cf2)
{
#ifdef NEED_AP_MLME
	int channel, chwidth, seg0_idx = 0, seg1_idx = 0;

	hostapd_logger(hapd, NULL, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO, "driver had channel switch: "
		       "freq=%d, ht=%d, offset=%d, width=%d, cf1=%d, cf2=%d",
		       freq, ht, offset, width, cf1, cf2);

	hapd->iface->freq = freq;

	channel = hostapd_hw_get_channel(hapd, freq);
	if (!channel) {
		hostapd_logger(hapd, NULL, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_WARNING, "driver switched to "
			       "bad channel!");
		return;
	}

	switch (width) {
	case CHAN_WIDTH_80:
		chwidth = VHT_CHANWIDTH_80MHZ;
		break;
	case CHAN_WIDTH_80P80:
		chwidth = VHT_CHANWIDTH_80P80MHZ;
		break;
	case CHAN_WIDTH_160:
		chwidth = VHT_CHANWIDTH_160MHZ;
		break;
	case CHAN_WIDTH_20_NOHT:
	case CHAN_WIDTH_20:
	case CHAN_WIDTH_40:
	default:
		chwidth = VHT_CHANWIDTH_USE_HT;
		break;
	}

	switch (hapd->iface->current_mode->mode) {
	case HOSTAPD_MODE_IEEE80211A:
		if (cf1 > 5000)
			seg0_idx = (cf1 - 5000) / 5;
		if (cf2 > 5000)
			seg1_idx = (cf2 - 5000) / 5;
		break;
	default:
		seg0_idx = hostapd_hw_get_channel(hapd, cf1);
		seg1_idx = hostapd_hw_get_channel(hapd, cf2);
		break;
	}

	hapd->iconf->channel = channel;
	hapd->iconf->ieee80211n = ht;
	hapd->iconf->secondary_channel = offset;
	hapd->iconf->vht_oper_chwidth = chwidth;
	hapd->iconf->vht_oper_centr_freq_seg0_idx = seg0_idx;
	hapd->iconf->vht_oper_centr_freq_seg1_idx = seg1_idx;

	if (hapd->csa_in_progress &&
	    freq == hapd->cs_freq_params.freq) {
		hostapd_cleanup_cs_params(hapd);
		ieee802_11_set_beacon(hapd);

		wpa_msg(hapd->msg_ctx, MSG_INFO, AP_CSA_FINISHED "freq=%d",
			freq);
	}
#endif /* NEED_AP_MLME */
}


void hostapd_event_connect_failed_reason(struct hostapd_data *hapd,
					 const u8 *addr, int reason_code)
{
	switch (reason_code) {
	case MAX_CLIENT_REACHED:
		wpa_msg(hapd->msg_ctx, MSG_INFO, AP_REJECTED_MAX_STA MACSTR,
			MAC2STR(addr));
		break;
	case BLOCKED_CLIENT:
		wpa_msg(hapd->msg_ctx, MSG_INFO, AP_REJECTED_BLOCKED_STA MACSTR,
			MAC2STR(addr));
		break;
	}
}


int hostapd_probe_req_rx(struct hostapd_data *hapd, const u8 *sa, const u8 *da,
			 const u8 *bssid, const u8 *ie, size_t ie_len,
			 int ssi_signal)
{
	size_t i;
	int ret = 0;

	if (sa == NULL || ie == NULL)
		return -1;

	random_add_randomness(sa, ETH_ALEN);
	for (i = 0; hapd->probereq_cb && i < hapd->num_probereq_cb; i++) {
		if (hapd->probereq_cb[i].cb(hapd->probereq_cb[i].ctx,
					    sa, da, bssid, ie, ie_len,
					    ssi_signal) > 0) {
			ret = 1;
			break;
		}
	}
	return ret;
}


#ifdef HOSTAPD

#ifdef CONFIG_IEEE80211R
static void hostapd_notify_auth_ft_finish(void *ctx, const u8 *dst,
					  const u8 *bssid,
					  u16 auth_transaction, u16 status,
					  const u8 *ies, size_t ies_len)
{
	struct hostapd_data *hapd = ctx;
	struct sta_info *sta;

	sta = ap_get_sta(hapd, dst);
	if (sta == NULL)
		return;

	hostapd_logger(hapd, dst, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG, "authentication OK (FT)");
	sta->flags |= WLAN_STA_AUTH;

	hostapd_sta_auth(hapd, dst, auth_transaction, status, ies, ies_len);
}
#endif /* CONFIG_IEEE80211R */


static void hostapd_notif_auth(struct hostapd_data *hapd,
			       struct auth_info *rx_auth)
{
	struct sta_info *sta;
	u16 status = WLAN_STATUS_SUCCESS;
	u8 resp_ies[2 + WLAN_AUTH_CHALLENGE_LEN];
	size_t resp_ies_len = 0;

	sta = ap_get_sta(hapd, rx_auth->peer);
	if (!sta) {
		sta = ap_sta_add(hapd, rx_auth->peer);
		if (sta == NULL) {
			status = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
			goto fail;
		}
	}
	sta->flags &= ~WLAN_STA_PREAUTH;
	ieee802_1x_notify_pre_auth(sta->eapol_sm, 0);
#ifdef CONFIG_IEEE80211R
	if (rx_auth->auth_type == WLAN_AUTH_FT && hapd->wpa_auth) {
		sta->auth_alg = WLAN_AUTH_FT;
		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr, NULL);
		if (sta->wpa_sm == NULL) {
			wpa_printf(MSG_DEBUG, "FT: Failed to initialize WPA "
				   "state machine");
			status = WLAN_STATUS_UNSPECIFIED_FAILURE;
			goto fail;
		}
		wpa_ft_process_auth(sta->wpa_sm, rx_auth->bssid,
				    rx_auth->auth_transaction, rx_auth->ies,
				    rx_auth->ies_len,
				    hostapd_notify_auth_ft_finish, hapd);
		return;
	}
#endif /* CONFIG_IEEE80211R */
fail:
	hostapd_sta_auth(hapd, rx_auth->peer, rx_auth->auth_transaction + 1,
			 status, resp_ies, resp_ies_len);
}


static void hostapd_action_rx(struct hostapd_data *hapd,
			      struct rx_mgmt *drv_mgmt)
{
	struct ieee80211_mgmt *mgmt;
	struct sta_info *sta;
	size_t plen __maybe_unused;
	u16 fc;

	if (drv_mgmt->frame_len < 24 + 1)
		return;

	plen = drv_mgmt->frame_len - 24 - 1;

	mgmt = (struct ieee80211_mgmt *) drv_mgmt->frame;
	fc = le_to_host16(mgmt->frame_control);
	if (WLAN_FC_GET_STYPE(fc) != WLAN_FC_STYPE_ACTION)
		return; /* handled by the driver */

        wpa_printf(MSG_DEBUG, "RX_ACTION cat %d action plen %d",
		   mgmt->u.action.category, (int) plen);

	sta = ap_get_sta(hapd, mgmt->sa);
	if (sta == NULL) {
		wpa_printf(MSG_DEBUG, "%s: station not found", __func__);
		return;
	}
#ifdef CONFIG_IEEE80211R
	if (mgmt->u.action.category == WLAN_ACTION_FT) {
		const u8 *payload = drv_mgmt->frame + 24 + 1;
		wpa_ft_action_rx(sta->wpa_sm, payload, plen);
	}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IEEE80211W
	if (mgmt->u.action.category == WLAN_ACTION_SA_QUERY && plen >= 4) {
		ieee802_11_sa_query_action(
			hapd, mgmt->sa,
			mgmt->u.action.u.sa_query_resp.action,
			mgmt->u.action.u.sa_query_resp.trans_id);
	}
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_WNM
	if (mgmt->u.action.category == WLAN_ACTION_WNM) {
		ieee802_11_rx_wnm_action_ap(hapd, mgmt, drv_mgmt->frame_len);
	}
#endif /* CONFIG_WNM */
}


#ifdef NEED_AP_MLME

#define HAPD_BROADCAST ((struct hostapd_data *) -1)

static struct hostapd_data * get_hapd_bssid(struct hostapd_iface *iface,
					    const u8 *bssid)
{
	size_t i;

	if (bssid == NULL)
		return NULL;
	if (bssid[0] == 0xff && bssid[1] == 0xff && bssid[2] == 0xff &&
	    bssid[3] == 0xff && bssid[4] == 0xff && bssid[5] == 0xff)
		return HAPD_BROADCAST;

	for (i = 0; i < iface->num_bss; i++) {
		if (os_memcmp(bssid, iface->bss[i]->own_addr, ETH_ALEN) == 0)
			return iface->bss[i];
	}

	return NULL;
}


static void hostapd_rx_from_unknown_sta(struct hostapd_data *hapd,
					const u8 *bssid, const u8 *addr,
					int wds)
{
	hapd = get_hapd_bssid(hapd->iface, bssid);
	if (hapd == NULL || hapd == HAPD_BROADCAST)
		return;

	ieee802_11_rx_from_unknown(hapd, addr, wds);
}


static int hostapd_mgmt_rx(struct hostapd_data *hapd, struct rx_mgmt *rx_mgmt)
{
	struct hostapd_iface *iface = hapd->iface;
	const struct ieee80211_hdr *hdr;
	const u8 *bssid;
	struct hostapd_frame_info fi;
	int ret;

#ifdef CONFIG_TESTING_OPTIONS
	if (hapd->ext_mgmt_frame_handling) {
		size_t hex_len = 2 * rx_mgmt->frame_len + 1;
		char *hex = os_malloc(hex_len);
		if (hex) {
			wpa_snprintf_hex(hex, hex_len, rx_mgmt->frame,
					 rx_mgmt->frame_len);
			wpa_msg(hapd->msg_ctx, MSG_INFO, "MGMT-RX %s", hex);
			os_free(hex);
		}
		return 1;
	}
#endif /* CONFIG_TESTING_OPTIONS */

	hdr = (const struct ieee80211_hdr *) rx_mgmt->frame;
	bssid = get_hdr_bssid(hdr, rx_mgmt->frame_len);
	if (bssid == NULL)
		return 0;

	hapd = get_hapd_bssid(iface, bssid);
	if (hapd == NULL) {
		u16 fc;
		fc = le_to_host16(hdr->frame_control);

		/*
		 * Drop frames to unknown BSSIDs except for Beacon frames which
		 * could be used to update neighbor information.
		 */
		if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
		    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_BEACON)
			hapd = iface->bss[0];
		else
			return 0;
	}

	os_memset(&fi, 0, sizeof(fi));
	fi.datarate = rx_mgmt->datarate;
	fi.ssi_signal = rx_mgmt->ssi_signal;

	if (hapd == HAPD_BROADCAST) {
		size_t i;
		ret = 0;
		for (i = 0; i < iface->num_bss; i++) {
			/* if bss is set, driver will call this function for
			 * each bss individually. */
			if (rx_mgmt->drv_priv &&
			    (iface->bss[i]->drv_priv != rx_mgmt->drv_priv))
				continue;

			if (ieee802_11_mgmt(iface->bss[i], rx_mgmt->frame,
					    rx_mgmt->frame_len, &fi) > 0)
				ret = 1;
		}
	} else
		ret = ieee802_11_mgmt(hapd, rx_mgmt->frame, rx_mgmt->frame_len,
				      &fi);

	random_add_randomness(&fi, sizeof(fi));

	return ret;
}


static void hostapd_mgmt_tx_cb(struct hostapd_data *hapd, const u8 *buf,
			       size_t len, u16 stype, int ok)
{
	struct ieee80211_hdr *hdr;
	hdr = (struct ieee80211_hdr *) buf;
	hapd = get_hapd_bssid(hapd->iface, get_hdr_bssid(hdr, len));
	if (hapd == NULL || hapd == HAPD_BROADCAST)
		return;
	ieee802_11_mgmt_cb(hapd, buf, len, stype, ok);
}

#endif /* NEED_AP_MLME */


static int hostapd_event_new_sta(struct hostapd_data *hapd, const u8 *addr)
{
	struct sta_info *sta = ap_get_sta(hapd, addr);
	if (sta)
		return 0;

	wpa_printf(MSG_DEBUG, "Data frame from unknown STA " MACSTR
		   " - adding a new STA", MAC2STR(addr));
	sta = ap_sta_add(hapd, addr);
	if (sta) {
		hostapd_new_assoc_sta(hapd, sta, 0);
	} else {
		wpa_printf(MSG_DEBUG, "Failed to add STA entry for " MACSTR,
			   MAC2STR(addr));
		return -1;
	}

	return 0;
}


static void hostapd_event_eapol_rx(struct hostapd_data *hapd, const u8 *src,
				   const u8 *data, size_t data_len)
{
	struct hostapd_iface *iface = hapd->iface;
	struct sta_info *sta;
	size_t j;

	for (j = 0; j < iface->num_bss; j++) {
		if ((sta = ap_get_sta(iface->bss[j], src))) {
			if (sta->flags & WLAN_STA_ASSOC) {
				hapd = iface->bss[j];
				break;
			}
		}
	}

	ieee802_1x_receive(hapd, src, data, data_len);
}


static struct hostapd_channel_data * hostapd_get_mode_channel(
	struct hostapd_iface *iface, unsigned int freq)
{
	int i;
	struct hostapd_channel_data *chan;

	for (i = 0; i < iface->current_mode->num_channels; i++) {
		chan = &iface->current_mode->channels[i];
		if (!chan)
			return NULL;
		if ((unsigned int) chan->freq == freq)
			return chan;
	}

	return NULL;
}


static void hostapd_update_nf(struct hostapd_iface *iface,
			      struct hostapd_channel_data *chan,
			      struct freq_survey *survey)
{
	if (!iface->chans_surveyed) {
		chan->min_nf = survey->nf;
		iface->lowest_nf = survey->nf;
	} else {
		if (dl_list_empty(&chan->survey_list))
			chan->min_nf = survey->nf;
		else if (survey->nf < chan->min_nf)
			chan->min_nf = survey->nf;
		if (survey->nf < iface->lowest_nf)
			iface->lowest_nf = survey->nf;
	}
}


static void hostapd_event_get_survey(struct hostapd_data *hapd,
				     struct survey_results *survey_results)
{
	struct hostapd_iface *iface = hapd->iface;
	struct freq_survey *survey, *tmp;
	struct hostapd_channel_data *chan;

	if (dl_list_empty(&survey_results->survey_list)) {
		wpa_printf(MSG_DEBUG, "No survey data received");
		return;
	}

	dl_list_for_each_safe(survey, tmp, &survey_results->survey_list,
			      struct freq_survey, list) {
		chan = hostapd_get_mode_channel(iface, survey->freq);
		if (!chan)
			continue;
		if (chan->flag & HOSTAPD_CHAN_DISABLED)
			continue;

		dl_list_del(&survey->list);
		dl_list_add_tail(&chan->survey_list, &survey->list);

		hostapd_update_nf(iface, chan, survey);

		iface->chans_surveyed++;
	}
}


#ifdef NEED_AP_MLME

static void hostapd_event_iface_unavailable(struct hostapd_data *hapd)
{
	wpa_printf(MSG_DEBUG, "Interface %s is unavailable -- stopped",
		   hapd->conf->iface);

	if (hapd->csa_in_progress) {
		wpa_printf(MSG_INFO, "CSA failed (%s was stopped)",
			   hapd->conf->iface);
		hostapd_switch_channel_fallback(hapd->iface,
						&hapd->cs_freq_params);
	}
}


static void hostapd_event_dfs_radar_detected(struct hostapd_data *hapd,
					     struct dfs_event *radar)
{
	wpa_printf(MSG_DEBUG, "DFS radar detected on %d MHz", radar->freq);
	hostapd_dfs_radar_detected(hapd->iface, radar->freq, radar->ht_enabled,
				   radar->chan_offset, radar->chan_width,
				   radar->cf1, radar->cf2);
}


static void hostapd_event_dfs_cac_finished(struct hostapd_data *hapd,
					   struct dfs_event *radar)
{
	wpa_printf(MSG_DEBUG, "DFS CAC finished on %d MHz", radar->freq);
	hostapd_dfs_complete_cac(hapd->iface, 1, radar->freq, radar->ht_enabled,
				 radar->chan_offset, radar->chan_width,
				 radar->cf1, radar->cf2);
}


static void hostapd_event_dfs_cac_aborted(struct hostapd_data *hapd,
					  struct dfs_event *radar)
{
	wpa_printf(MSG_DEBUG, "DFS CAC aborted on %d MHz", radar->freq);
	hostapd_dfs_complete_cac(hapd->iface, 0, radar->freq, radar->ht_enabled,
				 radar->chan_offset, radar->chan_width,
				 radar->cf1, radar->cf2);
}


static void hostapd_event_dfs_nop_finished(struct hostapd_data *hapd,
					   struct dfs_event *radar)
{
	wpa_printf(MSG_DEBUG, "DFS NOP finished on %d MHz", radar->freq);
	hostapd_dfs_nop_finished(hapd->iface, radar->freq, radar->ht_enabled,
				 radar->chan_offset, radar->chan_width,
				 radar->cf1, radar->cf2);
}

#endif /* NEED_AP_MLME */


void wpa_supplicant_event(void *ctx, enum wpa_event_type event,
			  union wpa_event_data *data)
{
	struct hostapd_data *hapd = ctx;
#ifndef CONFIG_NO_STDOUT_DEBUG
	int level = MSG_DEBUG;

	if (event == EVENT_RX_MGMT && data->rx_mgmt.frame &&
	    data->rx_mgmt.frame_len >= 24) {
		const struct ieee80211_hdr *hdr;
		u16 fc;
		hdr = (const struct ieee80211_hdr *) data->rx_mgmt.frame;
		fc = le_to_host16(hdr->frame_control);
		if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
		    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_BEACON)
			level = MSG_EXCESSIVE;
		if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
		    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_PROBE_REQ)
			level = MSG_EXCESSIVE;
	}

	wpa_dbg(hapd->msg_ctx, level, "Event %s (%d) received",
		event_to_string(event), event);
#endif /* CONFIG_NO_STDOUT_DEBUG */

	switch (event) {
	case EVENT_MICHAEL_MIC_FAILURE:
		michael_mic_failure(hapd, data->michael_mic_failure.src, 1);
		break;
	case EVENT_SCAN_RESULTS:
		if (hapd->iface->scan_cb)
			hapd->iface->scan_cb(hapd->iface);
		break;
#ifdef CONFIG_IEEE80211R
	case EVENT_FT_RRB_RX:
		wpa_ft_rrb_rx(hapd->wpa_auth, data->ft_rrb_rx.src,
			      data->ft_rrb_rx.data, data->ft_rrb_rx.data_len);
		break;
#endif /* CONFIG_IEEE80211R */
	case EVENT_WPS_BUTTON_PUSHED:
		hostapd_wps_button_pushed(hapd, NULL);
		break;
#ifdef NEED_AP_MLME
	case EVENT_TX_STATUS:
		switch (data->tx_status.type) {
		case WLAN_FC_TYPE_MGMT:
			hostapd_mgmt_tx_cb(hapd, data->tx_status.data,
					   data->tx_status.data_len,
					   data->tx_status.stype,
					   data->tx_status.ack);
			break;
		case WLAN_FC_TYPE_DATA:
			hostapd_tx_status(hapd, data->tx_status.dst,
					  data->tx_status.data,
					  data->tx_status.data_len,
					  data->tx_status.ack);
			break;
		}
		break;
	case EVENT_EAPOL_TX_STATUS:
		hostapd_eapol_tx_status(hapd, data->eapol_tx_status.dst,
					data->eapol_tx_status.data,
					data->eapol_tx_status.data_len,
					data->eapol_tx_status.ack);
		break;
	case EVENT_DRIVER_CLIENT_POLL_OK:
		hostapd_client_poll_ok(hapd, data->client_poll.addr);
		break;
	case EVENT_RX_FROM_UNKNOWN:
		hostapd_rx_from_unknown_sta(hapd, data->rx_from_unknown.bssid,
					    data->rx_from_unknown.addr,
					    data->rx_from_unknown.wds);
		break;
#endif /* NEED_AP_MLME */
	case EVENT_RX_MGMT:
		if (!data->rx_mgmt.frame)
			break;
#ifdef NEED_AP_MLME
		if (hostapd_mgmt_rx(hapd, &data->rx_mgmt) > 0)
			break;
#endif /* NEED_AP_MLME */
		hostapd_action_rx(hapd, &data->rx_mgmt);
		break;
	case EVENT_RX_PROBE_REQ:
		if (data->rx_probe_req.sa == NULL ||
		    data->rx_probe_req.ie == NULL)
			break;
		hostapd_probe_req_rx(hapd, data->rx_probe_req.sa,
				     data->rx_probe_req.da,
				     data->rx_probe_req.bssid,
				     data->rx_probe_req.ie,
				     data->rx_probe_req.ie_len,
				     data->rx_probe_req.ssi_signal);
		break;
	case EVENT_NEW_STA:
		hostapd_event_new_sta(hapd, data->new_sta.addr);
		break;
	case EVENT_EAPOL_RX:
		hostapd_event_eapol_rx(hapd, data->eapol_rx.src,
				       data->eapol_rx.data,
				       data->eapol_rx.data_len);
		break;
	case EVENT_ASSOC:
		if (!data)
			return;
		hostapd_notif_assoc(hapd, data->assoc_info.addr,
				    data->assoc_info.req_ies,
				    data->assoc_info.req_ies_len,
				    data->assoc_info.reassoc);
		break;
	case EVENT_DISASSOC:
		if (data)
			hostapd_notif_disassoc(hapd, data->disassoc_info.addr);
		break;
	case EVENT_DEAUTH:
		if (data)
			hostapd_notif_disassoc(hapd, data->deauth_info.addr);
		break;
	case EVENT_STATION_LOW_ACK:
		if (!data)
			break;
		hostapd_event_sta_low_ack(hapd, data->low_ack.addr);
		break;
	case EVENT_AUTH:
		hostapd_notif_auth(hapd, &data->auth);
		break;
	case EVENT_CH_SWITCH:
		if (!data)
			break;
		hostapd_event_ch_switch(hapd, data->ch_switch.freq,
					data->ch_switch.ht_enabled,
					data->ch_switch.ch_offset,
					data->ch_switch.ch_width,
					data->ch_switch.cf1,
					data->ch_switch.cf2);
		break;
	case EVENT_CONNECT_FAILED_REASON:
		if (!data)
			break;
		hostapd_event_connect_failed_reason(
			hapd, data->connect_failed_reason.addr,
			data->connect_failed_reason.code);
		break;
	case EVENT_SURVEY:
		hostapd_event_get_survey(hapd, &data->survey_results);
		break;
#ifdef NEED_AP_MLME
	case EVENT_INTERFACE_UNAVAILABLE:
		hostapd_event_iface_unavailable(hapd);
		break;
	case EVENT_DFS_RADAR_DETECTED:
		if (!data)
			break;
		hostapd_event_dfs_radar_detected(hapd, &data->dfs_event);
		break;
	case EVENT_DFS_CAC_FINISHED:
		if (!data)
			break;
		hostapd_event_dfs_cac_finished(hapd, &data->dfs_event);
		break;
	case EVENT_DFS_CAC_ABORTED:
		if (!data)
			break;
		hostapd_event_dfs_cac_aborted(hapd, &data->dfs_event);
		break;
	case EVENT_DFS_NOP_FINISHED:
		if (!data)
			break;
		hostapd_event_dfs_nop_finished(hapd, &data->dfs_event);
		break;
	case EVENT_CHANNEL_LIST_CHANGED:
		/* channel list changed (regulatory?), update channel list */
		/* TODO: check this. hostapd_get_hw_features() initializes
		 * too much stuff. */
		/* hostapd_get_hw_features(hapd->iface); */
		hostapd_channel_list_updated(
			hapd->iface, data->channel_list_changed.initiator);
		break;
#endif /* NEED_AP_MLME */
	default:
		wpa_printf(MSG_DEBUG, "Unknown event %d", event);
		break;
	}
}

#endif /* HOSTAPD */
