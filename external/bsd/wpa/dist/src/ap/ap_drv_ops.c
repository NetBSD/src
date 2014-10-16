/*
 * hostapd - Driver operations
 * Copyright (c) 2009-2010, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "common/ieee802_11_defs.h"
#include "wps/wps.h"
#include "p2p/p2p.h"
#include "hostapd.h"
#include "ieee802_11.h"
#include "sta_info.h"
#include "ap_config.h"
#include "p2p_hostapd.h"
#include "hs20.h"
#include "ap_drv_ops.h"


u32 hostapd_sta_flags_to_drv(u32 flags)
{
	int res = 0;
	if (flags & WLAN_STA_AUTHORIZED)
		res |= WPA_STA_AUTHORIZED;
	if (flags & WLAN_STA_WMM)
		res |= WPA_STA_WMM;
	if (flags & WLAN_STA_SHORT_PREAMBLE)
		res |= WPA_STA_SHORT_PREAMBLE;
	if (flags & WLAN_STA_MFP)
		res |= WPA_STA_MFP;
	return res;
}


int hostapd_build_ap_extra_ies(struct hostapd_data *hapd,
			       struct wpabuf **beacon_ret,
			       struct wpabuf **proberesp_ret,
			       struct wpabuf **assocresp_ret)
{
	struct wpabuf *beacon = NULL, *proberesp = NULL, *assocresp = NULL;
	u8 buf[200], *pos;

	*beacon_ret = *proberesp_ret = *assocresp_ret = NULL;

	pos = buf;
	pos = hostapd_eid_time_adv(hapd, pos);
	if (pos != buf) {
		if (wpabuf_resize(&beacon, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(beacon, buf, pos - buf);
	}
	pos = hostapd_eid_time_zone(hapd, pos);
	if (pos != buf) {
		if (wpabuf_resize(&proberesp, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(proberesp, buf, pos - buf);
	}

	pos = buf;
	pos = hostapd_eid_ext_capab(hapd, pos);
	if (pos != buf) {
		if (wpabuf_resize(&assocresp, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(assocresp, buf, pos - buf);
	}
	pos = hostapd_eid_interworking(hapd, pos);
	pos = hostapd_eid_adv_proto(hapd, pos);
	pos = hostapd_eid_roaming_consortium(hapd, pos);
	if (pos != buf) {
		if (wpabuf_resize(&beacon, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(beacon, buf, pos - buf);

		if (wpabuf_resize(&proberesp, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(proberesp, buf, pos - buf);
	}

	if (hapd->wps_beacon_ie) {
		if (wpabuf_resize(&beacon, wpabuf_len(hapd->wps_beacon_ie)) <
		    0)
			goto fail;
		wpabuf_put_buf(beacon, hapd->wps_beacon_ie);
	}

	if (hapd->wps_probe_resp_ie) {
		if (wpabuf_resize(&proberesp,
				  wpabuf_len(hapd->wps_probe_resp_ie)) < 0)
			goto fail;
		wpabuf_put_buf(proberesp, hapd->wps_probe_resp_ie);
	}

#ifdef CONFIG_P2P
	if (hapd->p2p_beacon_ie) {
		if (wpabuf_resize(&beacon, wpabuf_len(hapd->p2p_beacon_ie)) <
		    0)
			goto fail;
		wpabuf_put_buf(beacon, hapd->p2p_beacon_ie);
	}

	if (hapd->p2p_probe_resp_ie) {
		if (wpabuf_resize(&proberesp,
				  wpabuf_len(hapd->p2p_probe_resp_ie)) < 0)
			goto fail;
		wpabuf_put_buf(proberesp, hapd->p2p_probe_resp_ie);
	}
#endif /* CONFIG_P2P */

#ifdef CONFIG_P2P_MANAGER
	if (hapd->conf->p2p & P2P_MANAGE) {
		if (wpabuf_resize(&beacon, 100) == 0) {
			u8 *start, *p;
			start = wpabuf_put(beacon, 0);
			p = hostapd_eid_p2p_manage(hapd, start);
			wpabuf_put(beacon, p - start);
		}

		if (wpabuf_resize(&proberesp, 100) == 0) {
			u8 *start, *p;
			start = wpabuf_put(proberesp, 0);
			p = hostapd_eid_p2p_manage(hapd, start);
			wpabuf_put(proberesp, p - start);
		}
	}
#endif /* CONFIG_P2P_MANAGER */

#ifdef CONFIG_WPS
	if (hapd->conf->wps_state) {
		struct wpabuf *a = wps_build_assoc_resp_ie();
		if (a && wpabuf_resize(&assocresp, wpabuf_len(a)) == 0)
			wpabuf_put_buf(assocresp, a);
		wpabuf_free(a);
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P_MANAGER
	if (hapd->conf->p2p & P2P_MANAGE) {
		if (wpabuf_resize(&assocresp, 100) == 0) {
			u8 *start, *p;
			start = wpabuf_put(assocresp, 0);
			p = hostapd_eid_p2p_manage(hapd, start);
			wpabuf_put(assocresp, p - start);
		}
	}
#endif /* CONFIG_P2P_MANAGER */

#ifdef CONFIG_WIFI_DISPLAY
	if (hapd->p2p_group) {
		struct wpabuf *a;
		a = p2p_group_assoc_resp_ie(hapd->p2p_group, P2P_SC_SUCCESS);
		if (a && wpabuf_resize(&assocresp, wpabuf_len(a)) == 0)
			wpabuf_put_buf(assocresp, a);
		wpabuf_free(a);
	}
#endif /* CONFIG_WIFI_DISPLAY */

#ifdef CONFIG_HS20
	pos = buf;
	pos = hostapd_eid_hs20_indication(hapd, pos);
	if (pos != buf) {
		if (wpabuf_resize(&beacon, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(beacon, buf, pos - buf);

		if (wpabuf_resize(&proberesp, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(proberesp, buf, pos - buf);
	}

	pos = hostapd_eid_osen(hapd, buf);
	if (pos != buf) {
		if (wpabuf_resize(&beacon, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(beacon, buf, pos - buf);

		if (wpabuf_resize(&proberesp, pos - buf) != 0)
			goto fail;
		wpabuf_put_data(proberesp, buf, pos - buf);
	}
#endif /* CONFIG_HS20 */

	if (hapd->conf->vendor_elements) {
		size_t add = wpabuf_len(hapd->conf->vendor_elements);
		if (wpabuf_resize(&beacon, add) == 0)
			wpabuf_put_buf(beacon, hapd->conf->vendor_elements);
		if (wpabuf_resize(&proberesp, add) == 0)
			wpabuf_put_buf(proberesp, hapd->conf->vendor_elements);
	}

	*beacon_ret = beacon;
	*proberesp_ret = proberesp;
	*assocresp_ret = assocresp;

	return 0;

fail:
	wpabuf_free(beacon);
	wpabuf_free(proberesp);
	wpabuf_free(assocresp);
	return -1;
}


void hostapd_free_ap_extra_ies(struct hostapd_data *hapd,
			       struct wpabuf *beacon,
			       struct wpabuf *proberesp,
			       struct wpabuf *assocresp)
{
	wpabuf_free(beacon);
	wpabuf_free(proberesp);
	wpabuf_free(assocresp);
}


int hostapd_set_ap_wps_ie(struct hostapd_data *hapd)
{
	struct wpabuf *beacon, *proberesp, *assocresp;
	int ret;

	if (hapd->driver == NULL || hapd->driver->set_ap_wps_ie == NULL)
		return 0;

	if (hostapd_build_ap_extra_ies(hapd, &beacon, &proberesp, &assocresp) <
	    0)
		return -1;

	ret = hapd->driver->set_ap_wps_ie(hapd->drv_priv, beacon, proberesp,
					  assocresp);

	hostapd_free_ap_extra_ies(hapd, beacon, proberesp, assocresp);

	return ret;
}


int hostapd_set_authorized(struct hostapd_data *hapd,
			   struct sta_info *sta, int authorized)
{
	if (authorized) {
		return hostapd_sta_set_flags(hapd, sta->addr,
					     hostapd_sta_flags_to_drv(
						     sta->flags),
					     WPA_STA_AUTHORIZED, ~0);
	}

	return hostapd_sta_set_flags(hapd, sta->addr,
				     hostapd_sta_flags_to_drv(sta->flags),
				     0, ~WPA_STA_AUTHORIZED);
}


int hostapd_set_sta_flags(struct hostapd_data *hapd, struct sta_info *sta)
{
	int set_flags, total_flags, flags_and, flags_or;
	total_flags = hostapd_sta_flags_to_drv(sta->flags);
	set_flags = WPA_STA_SHORT_PREAMBLE | WPA_STA_WMM | WPA_STA_MFP;
	if (((!hapd->conf->ieee802_1x && !hapd->conf->wpa) ||
	     sta->auth_alg == WLAN_AUTH_FT) &&
	    sta->flags & WLAN_STA_AUTHORIZED)
		set_flags |= WPA_STA_AUTHORIZED;
	flags_or = total_flags & set_flags;
	flags_and = total_flags | ~set_flags;
	return hostapd_sta_set_flags(hapd, sta->addr, total_flags,
				     flags_or, flags_and);
}


int hostapd_set_drv_ieee8021x(struct hostapd_data *hapd, const char *ifname,
			      int enabled)
{
	struct wpa_bss_params params;
	os_memset(&params, 0, sizeof(params));
	params.ifname = ifname;
	params.enabled = enabled;
	if (enabled) {
		params.wpa = hapd->conf->wpa;
		params.ieee802_1x = hapd->conf->ieee802_1x;
		params.wpa_group = hapd->conf->wpa_group;
		params.wpa_pairwise = hapd->conf->wpa_pairwise |
			hapd->conf->rsn_pairwise;
		params.wpa_key_mgmt = hapd->conf->wpa_key_mgmt;
		params.rsn_preauth = hapd->conf->rsn_preauth;
#ifdef CONFIG_IEEE80211W
		params.ieee80211w = hapd->conf->ieee80211w;
#endif /* CONFIG_IEEE80211W */
	}
	return hostapd_set_ieee8021x(hapd, &params);
}


int hostapd_vlan_if_add(struct hostapd_data *hapd, const char *ifname)
{
	char force_ifname[IFNAMSIZ];
	u8 if_addr[ETH_ALEN];
	return hostapd_if_add(hapd, WPA_IF_AP_VLAN, ifname, hapd->own_addr,
			      NULL, NULL, force_ifname, if_addr, NULL, 0);
}


int hostapd_vlan_if_remove(struct hostapd_data *hapd, const char *ifname)
{
	return hostapd_if_remove(hapd, WPA_IF_AP_VLAN, ifname);
}


int hostapd_set_wds_sta(struct hostapd_data *hapd, char *ifname_wds,
			const u8 *addr, int aid, int val)
{
	const char *bridge = NULL;

	if (hapd->driver == NULL || hapd->driver->set_wds_sta == NULL)
		return -1;
	if (hapd->conf->wds_bridge[0])
		bridge = hapd->conf->wds_bridge;
	else if (hapd->conf->bridge[0])
		bridge = hapd->conf->bridge;
	return hapd->driver->set_wds_sta(hapd->drv_priv, addr, aid, val,
					 bridge, ifname_wds);
}


int hostapd_add_sta_node(struct hostapd_data *hapd, const u8 *addr,
			 u16 auth_alg)
{
	if (hapd->driver == NULL || hapd->driver->add_sta_node == NULL)
		return 0;
	return hapd->driver->add_sta_node(hapd->drv_priv, addr, auth_alg);
}


int hostapd_sta_auth(struct hostapd_data *hapd, const u8 *addr,
		     u16 seq, u16 status, const u8 *ie, size_t len)
{
	if (hapd->driver == NULL || hapd->driver->sta_auth == NULL)
		return 0;
	return hapd->driver->sta_auth(hapd->drv_priv, hapd->own_addr, addr,
				      seq, status, ie, len);
}


int hostapd_sta_assoc(struct hostapd_data *hapd, const u8 *addr,
		      int reassoc, u16 status, const u8 *ie, size_t len)
{
	if (hapd->driver == NULL || hapd->driver->sta_assoc == NULL)
		return 0;
	return hapd->driver->sta_assoc(hapd->drv_priv, hapd->own_addr, addr,
				       reassoc, status, ie, len);
}


int hostapd_sta_add(struct hostapd_data *hapd,
		    const u8 *addr, u16 aid, u16 capability,
		    const u8 *supp_rates, size_t supp_rates_len,
		    u16 listen_interval,
		    const struct ieee80211_ht_capabilities *ht_capab,
		    const struct ieee80211_vht_capabilities *vht_capab,
		    u32 flags, u8 qosinfo, u8 vht_opmode)
{
	struct hostapd_sta_add_params params;

	if (hapd->driver == NULL)
		return 0;
	if (hapd->driver->sta_add == NULL)
		return 0;

	os_memset(&params, 0, sizeof(params));
	params.addr = addr;
	params.aid = aid;
	params.capability = capability;
	params.supp_rates = supp_rates;
	params.supp_rates_len = supp_rates_len;
	params.listen_interval = listen_interval;
	params.ht_capabilities = ht_capab;
	params.vht_capabilities = vht_capab;
	params.vht_opmode_enabled = !!(flags & WLAN_STA_VHT_OPMODE_ENABLED);
	params.vht_opmode = vht_opmode;
	params.flags = hostapd_sta_flags_to_drv(flags);
	params.qosinfo = qosinfo;
	return hapd->driver->sta_add(hapd->drv_priv, &params);
}


int hostapd_add_tspec(struct hostapd_data *hapd, const u8 *addr,
		      u8 *tspec_ie, size_t tspec_ielen)
{
	if (hapd->driver == NULL || hapd->driver->add_tspec == NULL)
		return 0;
	return hapd->driver->add_tspec(hapd->drv_priv, addr, tspec_ie,
				       tspec_ielen);
}


int hostapd_set_privacy(struct hostapd_data *hapd, int enabled)
{
	if (hapd->driver == NULL || hapd->driver->set_privacy == NULL)
		return 0;
	return hapd->driver->set_privacy(hapd->drv_priv, enabled);
}


int hostapd_set_generic_elem(struct hostapd_data *hapd, const u8 *elem,
			     size_t elem_len)
{
	if (hapd->driver == NULL || hapd->driver->set_generic_elem == NULL)
		return 0;
	return hapd->driver->set_generic_elem(hapd->drv_priv, elem, elem_len);
}


int hostapd_get_ssid(struct hostapd_data *hapd, u8 *buf, size_t len)
{
	if (hapd->driver == NULL || hapd->driver->hapd_get_ssid == NULL)
		return 0;
	return hapd->driver->hapd_get_ssid(hapd->drv_priv, buf, len);
}


int hostapd_set_ssid(struct hostapd_data *hapd, const u8 *buf, size_t len)
{
	if (hapd->driver == NULL || hapd->driver->hapd_set_ssid == NULL)
		return 0;
	return hapd->driver->hapd_set_ssid(hapd->drv_priv, buf, len);
}


int hostapd_if_add(struct hostapd_data *hapd, enum wpa_driver_if_type type,
		   const char *ifname, const u8 *addr, void *bss_ctx,
		   void **drv_priv, char *force_ifname, u8 *if_addr,
		   const char *bridge, int use_existing)
{
	if (hapd->driver == NULL || hapd->driver->if_add == NULL)
		return -1;
	return hapd->driver->if_add(hapd->drv_priv, type, ifname, addr,
				    bss_ctx, drv_priv, force_ifname, if_addr,
				    bridge, use_existing);
}


int hostapd_if_remove(struct hostapd_data *hapd, enum wpa_driver_if_type type,
		      const char *ifname)
{
	if (hapd->driver == NULL || hapd->drv_priv == NULL ||
	    hapd->driver->if_remove == NULL)
		return -1;
	return hapd->driver->if_remove(hapd->drv_priv, type, ifname);
}


int hostapd_set_ieee8021x(struct hostapd_data *hapd,
			  struct wpa_bss_params *params)
{
	if (hapd->driver == NULL || hapd->driver->set_ieee8021x == NULL)
		return 0;
	return hapd->driver->set_ieee8021x(hapd->drv_priv, params);
}


int hostapd_get_seqnum(const char *ifname, struct hostapd_data *hapd,
		       const u8 *addr, int idx, u8 *seq)
{
	if (hapd->driver == NULL || hapd->driver->get_seqnum == NULL)
		return 0;
	return hapd->driver->get_seqnum(ifname, hapd->drv_priv, addr, idx,
					seq);
}


int hostapd_flush(struct hostapd_data *hapd)
{
	if (hapd->driver == NULL || hapd->driver->flush == NULL)
		return 0;
	return hapd->driver->flush(hapd->drv_priv);
}


int hostapd_set_freq_params(struct hostapd_freq_params *data, int mode,
			    int freq, int channel, int ht_enabled,
			    int vht_enabled, int sec_channel_offset,
			    int vht_oper_chwidth, int center_segment0,
			    int center_segment1, u32 vht_caps)
{
	int tmp;

	os_memset(data, 0, sizeof(*data));
	data->mode = mode;
	data->freq = freq;
	data->channel = channel;
	data->ht_enabled = ht_enabled;
	data->vht_enabled = vht_enabled;
	data->sec_channel_offset = sec_channel_offset;
	data->center_freq1 = freq + sec_channel_offset * 10;
	data->center_freq2 = 0;
	data->bandwidth = sec_channel_offset ? 40 : 20;

	/*
	 * This validation code is probably misplaced, maybe it should be
	 * in src/ap/hw_features.c and check the hardware support as well.
	 */
	if (data->vht_enabled) switch (vht_oper_chwidth) {
	case VHT_CHANWIDTH_USE_HT:
		if (center_segment1)
			return -1;
		if (center_segment0 != 0 &&
		    5000 + center_segment0 * 5 != data->center_freq1 &&
		    2407 + center_segment0 * 5 != data->center_freq1)
			return -1;
		break;
	case VHT_CHANWIDTH_80P80MHZ:
		if (!(vht_caps & VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ)) {
			wpa_printf(MSG_ERROR,
				   "80+80 channel width is not supported!");
			return -1;
		}
		if (center_segment1 == center_segment0 + 4 ||
		    center_segment1 == center_segment0 - 4)
			return -1;
		data->center_freq2 = 5000 + center_segment1 * 5;
		/* fall through */
	case VHT_CHANWIDTH_80MHZ:
		data->bandwidth = 80;
		if (vht_oper_chwidth == 1 && center_segment1)
			return -1;
		if (vht_oper_chwidth == 3 && !center_segment1)
			return -1;
		if (!sec_channel_offset)
			return -1;
		/* primary 40 part must match the HT configuration */
		tmp = (30 + freq - 5000 - center_segment0 * 5)/20;
		tmp /= 2;
		if (data->center_freq1 != 5000 +
					 center_segment0 * 5 - 20 + 40 * tmp)
			return -1;
		data->center_freq1 = 5000 + center_segment0 * 5;
		break;
	case VHT_CHANWIDTH_160MHZ:
		data->bandwidth = 160;
		if (!(vht_caps & (VHT_CAP_SUPP_CHAN_WIDTH_160MHZ |
				  VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ))) {
			wpa_printf(MSG_ERROR,
				   "160MHZ channel width is not supported!");
			return -1;
		}
		if (center_segment1)
			return -1;
		if (!sec_channel_offset)
			return -1;
		/* primary 40 part must match the HT configuration */
		tmp = (70 + freq - 5000 - center_segment0 * 5)/20;
		tmp /= 2;
		if (data->center_freq1 != 5000 +
					 center_segment0 * 5 - 60 + 40 * tmp)
			return -1;
		data->center_freq1 = 5000 + center_segment0 * 5;
		break;
	}

	return 0;
}


int hostapd_set_freq(struct hostapd_data *hapd, int mode, int freq,
		     int channel, int ht_enabled, int vht_enabled,
		     int sec_channel_offset, int vht_oper_chwidth,
		     int center_segment0, int center_segment1)
{
	struct hostapd_freq_params data;

	if (hostapd_set_freq_params(&data, mode, freq, channel, ht_enabled,
				    vht_enabled, sec_channel_offset,
				    vht_oper_chwidth,
				    center_segment0, center_segment1,
				    hapd->iface->current_mode->vht_capab))
		return -1;

	if (hapd->driver == NULL)
		return 0;
	if (hapd->driver->set_freq == NULL)
		return 0;
	return hapd->driver->set_freq(hapd->drv_priv, &data);
}

int hostapd_set_rts(struct hostapd_data *hapd, int rts)
{
	if (hapd->driver == NULL || hapd->driver->set_rts == NULL)
		return 0;
	return hapd->driver->set_rts(hapd->drv_priv, rts);
}


int hostapd_set_frag(struct hostapd_data *hapd, int frag)
{
	if (hapd->driver == NULL || hapd->driver->set_frag == NULL)
		return 0;
	return hapd->driver->set_frag(hapd->drv_priv, frag);
}


int hostapd_sta_set_flags(struct hostapd_data *hapd, u8 *addr,
			  int total_flags, int flags_or, int flags_and)
{
	if (hapd->driver == NULL || hapd->driver->sta_set_flags == NULL)
		return 0;
	return hapd->driver->sta_set_flags(hapd->drv_priv, addr, total_flags,
					   flags_or, flags_and);
}


int hostapd_set_country(struct hostapd_data *hapd, const char *country)
{
	if (hapd->driver == NULL ||
	    hapd->driver->set_country == NULL)
		return 0;
	return hapd->driver->set_country(hapd->drv_priv, country);
}


int hostapd_set_tx_queue_params(struct hostapd_data *hapd, int queue, int aifs,
				int cw_min, int cw_max, int burst_time)
{
	if (hapd->driver == NULL || hapd->driver->set_tx_queue_params == NULL)
		return 0;
	return hapd->driver->set_tx_queue_params(hapd->drv_priv, queue, aifs,
						 cw_min, cw_max, burst_time);
}


struct hostapd_hw_modes *
hostapd_get_hw_feature_data(struct hostapd_data *hapd, u16 *num_modes,
			    u16 *flags)
{
	if (hapd->driver == NULL ||
	    hapd->driver->get_hw_feature_data == NULL)
		return NULL;
	return hapd->driver->get_hw_feature_data(hapd->drv_priv, num_modes,
						 flags);
}


int hostapd_driver_commit(struct hostapd_data *hapd)
{
	if (hapd->driver == NULL || hapd->driver->commit == NULL)
		return 0;
	return hapd->driver->commit(hapd->drv_priv);
}


int hostapd_drv_none(struct hostapd_data *hapd)
{
	return hapd->driver && os_strcmp(hapd->driver->name, "none") == 0;
}


int hostapd_driver_scan(struct hostapd_data *hapd,
			struct wpa_driver_scan_params *params)
{
	if (hapd->driver && hapd->driver->scan2)
		return hapd->driver->scan2(hapd->drv_priv, params);
	return -1;
}


struct wpa_scan_results * hostapd_driver_get_scan_results(
	struct hostapd_data *hapd)
{
	if (hapd->driver && hapd->driver->get_scan_results2)
		return hapd->driver->get_scan_results2(hapd->drv_priv);
	return NULL;
}


int hostapd_driver_set_noa(struct hostapd_data *hapd, u8 count, int start,
			   int duration)
{
	if (hapd->driver && hapd->driver->set_noa)
		return hapd->driver->set_noa(hapd->drv_priv, count, start,
					     duration);
	return -1;
}


int hostapd_drv_set_key(const char *ifname, struct hostapd_data *hapd,
			enum wpa_alg alg, const u8 *addr,
			int key_idx, int set_tx,
			const u8 *seq, size_t seq_len,
			const u8 *key, size_t key_len)
{
	if (hapd->driver == NULL || hapd->driver->set_key == NULL)
		return 0;
	return hapd->driver->set_key(ifname, hapd->drv_priv, alg, addr,
				     key_idx, set_tx, seq, seq_len, key,
				     key_len);
}


int hostapd_drv_send_mlme(struct hostapd_data *hapd,
			  const void *msg, size_t len, int noack)
{
	if (hapd->driver == NULL || hapd->driver->send_mlme == NULL)
		return 0;
	return hapd->driver->send_mlme(hapd->drv_priv, msg, len, noack);
}


int hostapd_drv_sta_deauth(struct hostapd_data *hapd,
			   const u8 *addr, int reason)
{
	if (hapd->driver == NULL || hapd->driver->sta_deauth == NULL)
		return 0;
	return hapd->driver->sta_deauth(hapd->drv_priv, hapd->own_addr, addr,
					reason);
}


int hostapd_drv_sta_disassoc(struct hostapd_data *hapd,
			     const u8 *addr, int reason)
{
	if (hapd->driver == NULL || hapd->driver->sta_disassoc == NULL)
		return 0;
	return hapd->driver->sta_disassoc(hapd->drv_priv, hapd->own_addr, addr,
					  reason);
}


int hostapd_drv_wnm_oper(struct hostapd_data *hapd, enum wnm_oper oper,
			 const u8 *peer, u8 *buf, u16 *buf_len)
{
	if (hapd->driver == NULL || hapd->driver->wnm_oper == NULL)
		return -1;
	return hapd->driver->wnm_oper(hapd->drv_priv, oper, peer, buf,
				      buf_len);
}


int hostapd_drv_send_action(struct hostapd_data *hapd, unsigned int freq,
			    unsigned int wait, const u8 *dst, const u8 *data,
			    size_t len)
{
	if (hapd->driver == NULL || hapd->driver->send_action == NULL)
		return 0;
	return hapd->driver->send_action(hapd->drv_priv, freq, wait, dst,
					 hapd->own_addr, hapd->own_addr, data,
					 len, 0);
}


int hostapd_start_dfs_cac(struct hostapd_iface *iface, int mode, int freq,
			  int channel, int ht_enabled, int vht_enabled,
			  int sec_channel_offset, int vht_oper_chwidth,
			  int center_segment0, int center_segment1)
{
	struct hostapd_data *hapd = iface->bss[0];
	struct hostapd_freq_params data;
	int res;

	if (!hapd->driver || !hapd->driver->start_dfs_cac)
		return 0;

	if (!iface->conf->ieee80211h) {
		wpa_printf(MSG_ERROR, "Can't start DFS CAC, DFS functionality "
			   "is not enabled");
		return -1;
	}

	if (hostapd_set_freq_params(&data, mode, freq, channel, ht_enabled,
				    vht_enabled, sec_channel_offset,
				    vht_oper_chwidth, center_segment0,
				    center_segment1,
				    iface->current_mode->vht_capab)) {
		wpa_printf(MSG_ERROR, "Can't set freq params");
		return -1;
	}

	res = hapd->driver->start_dfs_cac(hapd->drv_priv, &data);
	if (!res) {
		iface->cac_started = 1;
		os_get_reltime(&iface->dfs_cac_start);
	}

	return res;
}


int hostapd_drv_set_qos_map(struct hostapd_data *hapd,
			    const u8 *qos_map_set, u8 qos_map_set_len)
{
	if (hapd->driver == NULL || hapd->driver->set_qos_map == NULL)
		return 0;
	return hapd->driver->set_qos_map(hapd->drv_priv, qos_map_set,
					 qos_map_set_len);
}
