/*
 * wpa_supplicant - P2P
 * Copyright (c) 2009-2010, Atheros Communications
 * Copyright (c) 2010-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "eloop.h"
#include "common/ieee802_11_common.h"
#include "common/ieee802_11_defs.h"
#include "common/wpa_ctrl.h"
#include "wps/wps_i.h"
#include "p2p/p2p.h"
#include "ap/hostapd.h"
#include "ap/ap_config.h"
#include "ap/sta_info.h"
#include "ap/ap_drv_ops.h"
#include "ap/wps_hostapd.h"
#include "ap/p2p_hostapd.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "rsn_supp/wpa.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "ap.h"
#include "config_ssid.h"
#include "config.h"
#include "notify.h"
#include "scan.h"
#include "bss.h"
#include "offchannel.h"
#include "wps_supplicant.h"
#include "p2p_supplicant.h"
#include "wifi_display.h"


/*
 * How many times to try to scan to find the GO before giving up on join
 * request.
 */
#define P2P_MAX_JOIN_SCAN_ATTEMPTS 10

#define P2P_AUTO_PD_SCAN_ATTEMPTS 5

#ifndef P2P_MAX_CLIENT_IDLE
/*
 * How many seconds to try to reconnect to the GO when connection in P2P client
 * role has been lost.
 */
#define P2P_MAX_CLIENT_IDLE 10
#endif /* P2P_MAX_CLIENT_IDLE */

#ifndef P2P_MAX_INITIAL_CONN_WAIT
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * WPS provisioning step or after the re-invocation of a persistent group on a
 * P2P Client.
 */
#define P2P_MAX_INITIAL_CONN_WAIT 10
#endif /* P2P_MAX_INITIAL_CONN_WAIT */

#ifndef P2P_MAX_INITIAL_CONN_WAIT_GO
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * WPS provisioning step on the GO. This controls the extra time the P2P
 * operation is considered to be in progress (e.g., to delay other scans) after
 * WPS provisioning has been completed on the GO during group formation.
 */
#define P2P_MAX_INITIAL_CONN_WAIT_GO 10
#endif /* P2P_MAX_INITIAL_CONN_WAIT_GO */

#ifndef P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * re-invocation of a persistent group on the GO when the client is expected
 * to connect automatically (no user interaction).
 */
#define P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE 15
#endif /* P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE */

#define P2P_MGMT_DEVICE_PREFIX		"p2p-dev-"

enum p2p_group_removal_reason {
	P2P_GROUP_REMOVAL_UNKNOWN,
	P2P_GROUP_REMOVAL_SILENT,
	P2P_GROUP_REMOVAL_FORMATION_FAILED,
	P2P_GROUP_REMOVAL_REQUESTED,
	P2P_GROUP_REMOVAL_IDLE_TIMEOUT,
	P2P_GROUP_REMOVAL_UNAVAILABLE,
	P2P_GROUP_REMOVAL_GO_ENDING_SESSION,
	P2P_GROUP_REMOVAL_PSK_FAILURE,
	P2P_GROUP_REMOVAL_FREQ_CONFLICT
};


static void wpas_p2p_long_listen_timeout(void *eloop_ctx, void *timeout_ctx);
static struct wpa_supplicant *
wpas_p2p_get_group_iface(struct wpa_supplicant *wpa_s, int addr_allocated,
			 int go);
static int wpas_p2p_join_start(struct wpa_supplicant *wpa_s, int freq,
			       const u8 *ssid, size_t ssid_len);
static void wpas_p2p_join_scan_req(struct wpa_supplicant *wpa_s, int freq,
				   const u8 *ssid, size_t ssid_len);
static void wpas_p2p_join_scan(void *eloop_ctx, void *timeout_ctx);
static int wpas_p2p_join(struct wpa_supplicant *wpa_s, const u8 *iface_addr,
			 const u8 *dev_addr, enum p2p_wps_method wps_method,
			 int auto_join, int freq,
			 const u8 *ssid, size_t ssid_len);
static int wpas_p2p_create_iface(struct wpa_supplicant *wpa_s);
static void wpas_p2p_cross_connect_setup(struct wpa_supplicant *wpa_s);
static void wpas_p2p_group_idle_timeout(void *eloop_ctx, void *timeout_ctx);
static void wpas_p2p_set_group_idle_timeout(struct wpa_supplicant *wpa_s);
static void wpas_p2p_group_formation_timeout(void *eloop_ctx,
					     void *timeout_ctx);
static void wpas_p2p_group_freq_conflict(void *eloop_ctx, void *timeout_ctx);
static void wpas_p2p_fallback_to_go_neg(struct wpa_supplicant *wpa_s,
					int group_added);
static int wpas_p2p_stop_find_oper(struct wpa_supplicant *wpa_s);
static void wpas_stop_listen(void *ctx);
static void wpas_p2p_psk_failure_removal(void *eloop_ctx, void *timeout_ctx);
static void wpas_p2p_group_deinit(struct wpa_supplicant *wpa_s);


/*
 * Get the number of concurrent channels that the HW can operate, but that are
 * currently not in use by any of the wpa_supplicant interfaces.
 */
static int wpas_p2p_num_unused_channels(struct wpa_supplicant *wpa_s)
{
	int *freqs;
	int num, unused;

	freqs = os_calloc(wpa_s->num_multichan_concurrent, sizeof(int));
	if (!freqs)
		return -1;

	num = get_shared_radio_freqs(wpa_s, freqs,
				     wpa_s->num_multichan_concurrent);
	os_free(freqs);

	unused = wpa_s->num_multichan_concurrent - num;
	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: num_unused_channels: %d", unused);
	return unused;
}


/*
 * Get the frequencies that are currently in use by one or more of the virtual
 * interfaces, and that are also valid for P2P operation.
 */
static unsigned int
wpas_p2p_valid_oper_freqs(struct wpa_supplicant *wpa_s,
			  struct wpa_used_freq_data *p2p_freqs,
			  unsigned int len)
{
	struct wpa_used_freq_data *freqs;
	unsigned int num, i, j;

	freqs = os_calloc(wpa_s->num_multichan_concurrent,
			  sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return 0;

	num = get_shared_radio_freqs_data(wpa_s, freqs,
					  wpa_s->num_multichan_concurrent);

	os_memset(p2p_freqs, 0, sizeof(struct wpa_used_freq_data) * len);

	for (i = 0, j = 0; i < num && j < len; i++) {
		if (p2p_supported_freq(wpa_s->global->p2p, freqs[i].freq))
			p2p_freqs[j++] = freqs[i];
	}

	os_free(freqs);

	dump_freq_data(wpa_s, "valid for P2P", p2p_freqs, j);

	return j;
}


static void wpas_p2p_set_own_freq_preference(struct wpa_supplicant *wpa_s,
					     int freq)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;
	if (wpa_s->parent->conf->p2p_ignore_shared_freq &&
	    freq > 0 && wpa_s->num_multichan_concurrent > 1 &&
	    wpas_p2p_num_unused_channels(wpa_s) > 0) {
		wpa_printf(MSG_DEBUG, "P2P: Ignore own channel preference %d MHz due to p2p_ignore_shared_freq=1 configuration",
			   freq);
		freq = 0;
	}
	p2p_set_own_freq_preference(wpa_s->global->p2p, freq);
}


static void wpas_p2p_scan_res_handler(struct wpa_supplicant *wpa_s,
				      struct wpa_scan_results *scan_res)
{
	size_t i;

	if (wpa_s->p2p_scan_work) {
		struct wpa_radio_work *work = wpa_s->p2p_scan_work;
		wpa_s->p2p_scan_work = NULL;
		radio_work_done(work);
	}

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;

	wpa_printf(MSG_DEBUG, "P2P: Scan results received (%d BSS)",
		   (int) scan_res->num);

	for (i = 0; i < scan_res->num; i++) {
		struct wpa_scan_res *bss = scan_res->res[i];
		struct os_reltime time_tmp_age, entry_ts;
		const u8 *ies;
		size_t ies_len;

		time_tmp_age.sec = bss->age / 1000;
		time_tmp_age.usec = (bss->age % 1000) * 1000;
		os_reltime_sub(&scan_res->fetch_time, &time_tmp_age, &entry_ts);

		ies = (const u8 *) (bss + 1);
		ies_len = bss->ie_len;
		if (bss->beacon_ie_len > 0 &&
		    !wpa_scan_get_vendor_ie(bss, P2P_IE_VENDOR_TYPE) &&
		    wpa_scan_get_vendor_ie_beacon(bss, P2P_IE_VENDOR_TYPE)) {
			wpa_printf(MSG_DEBUG, "P2P: Use P2P IE(s) from Beacon frame since no P2P IE(s) in Probe Response frames received for "
				   MACSTR, MAC2STR(bss->bssid));
			ies = ies + ies_len;
			ies_len = bss->beacon_ie_len;
		}


		if (p2p_scan_res_handler(wpa_s->global->p2p, bss->bssid,
					 bss->freq, &entry_ts, bss->level,
					 ies, ies_len) > 0)
			break;
	}

	p2p_scan_res_handled(wpa_s->global->p2p);
}


static void wpas_p2p_trigger_scan_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct wpa_driver_scan_params *params = work->ctx;
	int ret;

	if (deinit) {
		if (!work->started) {
			wpa_scan_free_params(params);
			return;
		}

		wpa_s->p2p_scan_work = NULL;
		return;
	}

	ret = wpa_drv_scan(wpa_s, params);
	wpa_scan_free_params(params);
	work->ctx = NULL;
	if (ret) {
		radio_work_done(work);
		return;
	}

	os_get_reltime(&wpa_s->scan_trigger_time);
	wpa_s->scan_res_handler = wpas_p2p_scan_res_handler;
	wpa_s->own_scan_requested = 1;
	wpa_s->p2p_scan_work = work;
}


static int wpas_p2p_scan(void *ctx, enum p2p_scan_type type, int freq,
			 unsigned int num_req_dev_types,
			 const u8 *req_dev_types, const u8 *dev_id, u16 pw_id)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_driver_scan_params *params = NULL;
	struct wpabuf *wps_ie, *ies;
	unsigned int num_channels = 0;
	int social_channels_freq[] = { 2412, 2437, 2462, 60480 };
	size_t ielen;
	u8 *n, i;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (wpa_s->p2p_scan_work) {
		wpa_dbg(wpa_s, MSG_INFO, "P2P: Reject scan trigger since one is already pending");
		return -1;
	}

	params = os_zalloc(sizeof(*params));
	if (params == NULL)
		return -1;

	/* P2P Wildcard SSID */
	params->num_ssids = 1;
	n = os_malloc(P2P_WILDCARD_SSID_LEN);
	if (n == NULL)
		goto fail;
	os_memcpy(n, P2P_WILDCARD_SSID, P2P_WILDCARD_SSID_LEN);
	params->ssids[0].ssid = n;
	params->ssids[0].ssid_len = P2P_WILDCARD_SSID_LEN;

	wpa_s->wps->dev.p2p = 1;
	wps_ie = wps_build_probe_req_ie(pw_id, &wpa_s->wps->dev,
					wpa_s->wps->uuid, WPS_REQ_ENROLLEE,
					num_req_dev_types, req_dev_types);
	if (wps_ie == NULL)
		goto fail;

	ielen = p2p_scan_ie_buf_len(wpa_s->global->p2p);
	ies = wpabuf_alloc(wpabuf_len(wps_ie) + ielen);
	if (ies == NULL) {
		wpabuf_free(wps_ie);
		goto fail;
	}
	wpabuf_put_buf(ies, wps_ie);
	wpabuf_free(wps_ie);

	p2p_scan_ie(wpa_s->global->p2p, ies, dev_id);

	params->p2p_probe = 1;
	n = os_malloc(wpabuf_len(ies));
	if (n == NULL) {
		wpabuf_free(ies);
		goto fail;
	}
	os_memcpy(n, wpabuf_head(ies), wpabuf_len(ies));
	params->extra_ies = n;
	params->extra_ies_len = wpabuf_len(ies);
	wpabuf_free(ies);

	switch (type) {
	case P2P_SCAN_SOCIAL:
		params->freqs = os_calloc(ARRAY_SIZE(social_channels_freq) + 1,
					  sizeof(int));
		if (params->freqs == NULL)
			goto fail;
		for (i = 0; i < ARRAY_SIZE(social_channels_freq); i++) {
			if (p2p_supported_freq(wpa_s->global->p2p,
					       social_channels_freq[i]))
				params->freqs[num_channels++] =
					social_channels_freq[i];
		}
		params->freqs[num_channels++] = 0;
		break;
	case P2P_SCAN_FULL:
		break;
	case P2P_SCAN_SOCIAL_PLUS_ONE:
		params->freqs = os_calloc(ARRAY_SIZE(social_channels_freq) + 2,
					  sizeof(int));
		if (params->freqs == NULL)
			goto fail;
		for (i = 0; i < ARRAY_SIZE(social_channels_freq); i++) {
			if (p2p_supported_freq(wpa_s->global->p2p,
					       social_channels_freq[i]))
				params->freqs[num_channels++] =
					social_channels_freq[i];
		}
		if (p2p_supported_freq(wpa_s->global->p2p, freq))
			params->freqs[num_channels++] = freq;
		params->freqs[num_channels++] = 0;
		break;
	}

	radio_remove_works(wpa_s, "p2p-scan", 0);
	if (radio_add_work(wpa_s, 0, "p2p-scan", 0, wpas_p2p_trigger_scan_cb,
			   params) < 0)
		goto fail;
	return 0;

fail:
	wpa_scan_free_params(params);
	return -1;
}


static enum wpa_driver_if_type wpas_p2p_if_type(int p2p_group_interface)
{
	switch (p2p_group_interface) {
	case P2P_GROUP_INTERFACE_PENDING:
		return WPA_IF_P2P_GROUP;
	case P2P_GROUP_INTERFACE_GO:
		return WPA_IF_P2P_GO;
	case P2P_GROUP_INTERFACE_CLIENT:
		return WPA_IF_P2P_CLIENT;
	}

	return WPA_IF_P2P_GROUP;
}


static struct wpa_supplicant * wpas_get_p2p_group(struct wpa_supplicant *wpa_s,
						  const u8 *ssid,
						  size_t ssid_len, int *go)
{
	struct wpa_ssid *s;

	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		for (s = wpa_s->conf->ssid; s; s = s->next) {
			if (s->disabled != 0 || !s->p2p_group ||
			    s->ssid_len != ssid_len ||
			    os_memcmp(ssid, s->ssid, ssid_len) != 0)
				continue;
			if (s->mode == WPAS_MODE_P2P_GO &&
			    s != wpa_s->current_ssid)
				continue;
			if (go)
				*go = s->mode == WPAS_MODE_P2P_GO;
			return wpa_s;
		}
	}

	return NULL;
}


static int wpas_p2p_group_delete(struct wpa_supplicant *wpa_s,
				 enum p2p_group_removal_reason removal_reason)
{
	struct wpa_ssid *ssid;
	char *gtype;
	const char *reason;

	ssid = wpa_s->current_ssid;
	if (ssid == NULL) {
		/*
		 * The current SSID was not known, but there may still be a
		 * pending P2P group interface waiting for provisioning or a
		 * P2P group that is trying to reconnect.
		 */
		ssid = wpa_s->conf->ssid;
		while (ssid) {
			if (ssid->p2p_group && ssid->disabled != 2)
				break;
			ssid = ssid->next;
		}
		if (ssid == NULL &&
			wpa_s->p2p_group_interface == NOT_P2P_GROUP_INTERFACE)
		{
			wpa_printf(MSG_ERROR, "P2P: P2P group interface "
				   "not found");
			return -1;
		}
	}
	if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_GO)
		gtype = "GO";
	else if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT ||
		 (ssid && ssid->mode == WPAS_MODE_INFRA)) {
		wpa_s->reassociate = 0;
		wpa_s->disconnected = 1;
		gtype = "client";
	} else
		gtype = "GO";

	if (removal_reason != P2P_GROUP_REMOVAL_SILENT && ssid)
		wpas_notify_p2p_group_removed(wpa_s, ssid, gtype);

	if (os_strcmp(gtype, "client") == 0)
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);

	if (wpa_s->cross_connect_in_use) {
		wpa_s->cross_connect_in_use = 0;
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_CROSS_CONNECT_DISABLE "%s %s",
			       wpa_s->ifname, wpa_s->cross_connect_uplink);
	}
	switch (removal_reason) {
	case P2P_GROUP_REMOVAL_REQUESTED:
		reason = " reason=REQUESTED";
		break;
	case P2P_GROUP_REMOVAL_FORMATION_FAILED:
		reason = " reason=FORMATION_FAILED";
		break;
	case P2P_GROUP_REMOVAL_IDLE_TIMEOUT:
		reason = " reason=IDLE";
		break;
	case P2P_GROUP_REMOVAL_UNAVAILABLE:
		reason = " reason=UNAVAILABLE";
		break;
	case P2P_GROUP_REMOVAL_GO_ENDING_SESSION:
		reason = " reason=GO_ENDING_SESSION";
		break;
	case P2P_GROUP_REMOVAL_PSK_FAILURE:
		reason = " reason=PSK_FAILURE";
		break;
	case P2P_GROUP_REMOVAL_FREQ_CONFLICT:
		reason = " reason=FREQ_CONFLICT";
		break;
	default:
		reason = "";
		break;
	}
	if (removal_reason != P2P_GROUP_REMOVAL_SILENT) {
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_GROUP_REMOVED "%s %s%s",
			       wpa_s->ifname, gtype, reason);
	}

	if (eloop_cancel_timeout(wpas_p2p_group_freq_conflict, wpa_s, NULL) > 0)
		wpa_printf(MSG_DEBUG, "P2P: Cancelled P2P group freq_conflict timeout");
	if (eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL) > 0)
		wpa_printf(MSG_DEBUG, "P2P: Cancelled P2P group idle timeout");
	if (eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				 wpa_s->parent, NULL) > 0) {
		wpa_printf(MSG_DEBUG, "P2P: Cancelled P2P group formation "
			   "timeout");
		wpa_s->p2p_in_provisioning = 0;
	}

	wpa_s->p2p_in_invitation = 0;

	/*
	 * Make sure wait for the first client does not remain active after the
	 * group has been removed.
	 */
	wpa_s->global->p2p_go_wait_client.sec = 0;

	if (wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE) {
		struct wpa_global *global;
		char *ifname;
		enum wpa_driver_if_type type;
		wpa_printf(MSG_DEBUG, "P2P: Remove group interface %s",
			wpa_s->ifname);
		global = wpa_s->global;
		ifname = os_strdup(wpa_s->ifname);
		type = wpas_p2p_if_type(wpa_s->p2p_group_interface);
		wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
		wpa_s = global->ifaces;
		if (wpa_s && ifname)
			wpa_drv_if_remove(wpa_s, type, ifname);
		os_free(ifname);
		return 1;
	}

	if (!wpa_s->p2p_go_group_formation_completed) {
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
	}

	wpa_s->show_group_started = 0;
	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;

	wpa_s->waiting_presence_resp = 0;

	wpa_printf(MSG_DEBUG, "P2P: Remove temporary group network");
	if (ssid && (ssid->p2p_group ||
		     ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION ||
		     (ssid->key_mgmt & WPA_KEY_MGMT_WPS))) {
		int id = ssid->id;
		if (ssid == wpa_s->current_ssid) {
			wpa_sm_set_config(wpa_s->wpa, NULL);
			eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
			wpa_s->current_ssid = NULL;
		}
		/*
		 * Networks objects created during any P2P activities are not
		 * exposed out as they might/will confuse certain non-P2P aware
		 * applications since these network objects won't behave like
		 * regular ones.
		 *
		 * Likewise, we don't send out network removed signals for such
		 * network objects.
		 */
		wpa_config_remove_network(wpa_s->conf, id);
		wpa_supplicant_clear_status(wpa_s);
		wpa_supplicant_cancel_sched_scan(wpa_s);
	} else {
		wpa_printf(MSG_DEBUG, "P2P: Temporary group network not "
			   "found");
	}
	if (wpa_s->ap_iface)
		wpa_supplicant_ap_deinit(wpa_s);
	else
		wpa_drv_deinit_p2p_cli(wpa_s);

	return 0;
}


static int wpas_p2p_persistent_group(struct wpa_supplicant *wpa_s,
				     u8 *go_dev_addr,
				     const u8 *ssid, size_t ssid_len)
{
	struct wpa_bss *bss;
	const u8 *bssid;
	struct wpabuf *p2p;
	u8 group_capab;
	const u8 *addr;

	if (wpa_s->go_params)
		bssid = wpa_s->go_params->peer_interface_addr;
	else
		bssid = wpa_s->bssid;

	bss = wpa_bss_get(wpa_s, bssid, ssid, ssid_len);
	if (bss == NULL && wpa_s->go_params &&
	    !is_zero_ether_addr(wpa_s->go_params->peer_device_addr))
		bss = wpa_bss_get_p2p_dev_addr(
			wpa_s, wpa_s->go_params->peer_device_addr);
	if (bss == NULL) {
		u8 iface_addr[ETH_ALEN];
		if (p2p_get_interface_addr(wpa_s->global->p2p, bssid,
					   iface_addr) == 0)
			bss = wpa_bss_get(wpa_s, iface_addr, ssid, ssid_len);
	}
	if (bss == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Could not figure out whether "
			   "group is persistent - BSS " MACSTR " not found",
			   MAC2STR(bssid));
		return 0;
	}

	p2p = wpa_bss_get_vendor_ie_multi(bss, P2P_IE_VENDOR_TYPE);
	if (p2p == NULL)
		p2p = wpa_bss_get_vendor_ie_multi_beacon(bss,
							 P2P_IE_VENDOR_TYPE);
	if (p2p == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Could not figure out whether "
			   "group is persistent - BSS " MACSTR
			   " did not include P2P IE", MAC2STR(bssid));
		wpa_hexdump(MSG_DEBUG, "P2P: Probe Response IEs",
			    (u8 *) (bss + 1), bss->ie_len);
		wpa_hexdump(MSG_DEBUG, "P2P: Beacon IEs",
			    ((u8 *) bss + 1) + bss->ie_len,
			    bss->beacon_ie_len);
		return 0;
	}

	group_capab = p2p_get_group_capab(p2p);
	addr = p2p_get_go_dev_addr(p2p);
	wpa_printf(MSG_DEBUG, "P2P: Checking whether group is persistent: "
		   "group_capab=0x%x", group_capab);
	if (addr) {
		os_memcpy(go_dev_addr, addr, ETH_ALEN);
		wpa_printf(MSG_DEBUG, "P2P: GO Device Address " MACSTR,
			   MAC2STR(addr));
	} else
		os_memset(go_dev_addr, 0, ETH_ALEN);
	wpabuf_free(p2p);

	wpa_printf(MSG_DEBUG, "P2P: BSS " MACSTR " group_capab=0x%x "
		   "go_dev_addr=" MACSTR,
		   MAC2STR(bssid), group_capab, MAC2STR(go_dev_addr));

	return group_capab & P2P_GROUP_CAPAB_PERSISTENT_GROUP;
}


static int wpas_p2p_store_persistent_group(struct wpa_supplicant *wpa_s,
					   struct wpa_ssid *ssid,
					   const u8 *go_dev_addr)
{
	struct wpa_ssid *s;
	int changed = 0;

	wpa_printf(MSG_DEBUG, "P2P: Storing credentials for a persistent "
		   "group (GO Dev Addr " MACSTR ")", MAC2STR(go_dev_addr));
	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    os_memcmp(go_dev_addr, s->bssid, ETH_ALEN) == 0 &&
		    s->ssid_len == ssid->ssid_len &&
		    os_memcmp(ssid->ssid, s->ssid, ssid->ssid_len) == 0)
			break;
	}

	if (s) {
		wpa_printf(MSG_DEBUG, "P2P: Update existing persistent group "
			   "entry");
		if (ssid->passphrase && !s->passphrase)
			changed = 1;
		else if (ssid->passphrase && s->passphrase &&
			 os_strcmp(ssid->passphrase, s->passphrase) != 0)
			changed = 1;
	} else {
		wpa_printf(MSG_DEBUG, "P2P: Create a new persistent group "
			   "entry");
		changed = 1;
		s = wpa_config_add_network(wpa_s->conf);
		if (s == NULL)
			return -1;

		/*
		 * Instead of network_added we emit persistent_group_added
		 * notification. Also to keep the defense checks in
		 * persistent_group obj registration method, we set the
		 * relevant flags in s to designate it as a persistent group.
		 */
		s->p2p_group = 1;
		s->p2p_persistent_group = 1;
		wpas_notify_persistent_group_added(wpa_s, s);
		wpa_config_set_network_defaults(s);
	}

	s->p2p_group = 1;
	s->p2p_persistent_group = 1;
	s->disabled = 2;
	s->bssid_set = 1;
	os_memcpy(s->bssid, go_dev_addr, ETH_ALEN);
	s->mode = ssid->mode;
	s->auth_alg = WPA_AUTH_ALG_OPEN;
	s->key_mgmt = WPA_KEY_MGMT_PSK;
	s->proto = WPA_PROTO_RSN;
	s->pairwise_cipher = WPA_CIPHER_CCMP;
	s->export_keys = 1;
	if (ssid->passphrase) {
		os_free(s->passphrase);
		s->passphrase = os_strdup(ssid->passphrase);
	}
	if (ssid->psk_set) {
		s->psk_set = 1;
		os_memcpy(s->psk, ssid->psk, 32);
	}
	if (s->passphrase && !s->psk_set)
		wpa_config_update_psk(s);
	if (s->ssid == NULL || s->ssid_len < ssid->ssid_len) {
		os_free(s->ssid);
		s->ssid = os_malloc(ssid->ssid_len);
	}
	if (s->ssid) {
		s->ssid_len = ssid->ssid_len;
		os_memcpy(s->ssid, ssid->ssid, s->ssid_len);
	}
	if (ssid->mode == WPAS_MODE_P2P_GO && wpa_s->global->add_psk) {
		dl_list_add(&s->psk_list, &wpa_s->global->add_psk->list);
		wpa_s->global->add_psk = NULL;
		changed = 1;
	}

	if (changed && wpa_s->conf->update_config &&
	    wpa_config_write(wpa_s->confname, wpa_s->conf)) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to update configuration");
	}

	return s->id;
}


static void wpas_p2p_add_persistent_group_client(struct wpa_supplicant *wpa_s,
						 const u8 *addr)
{
	struct wpa_ssid *ssid, *s;
	u8 *n;
	size_t i;
	int found = 0;

	ssid = wpa_s->current_ssid;
	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GO ||
	    !ssid->p2p_persistent_group)
		return;

	for (s = wpa_s->parent->conf->ssid; s; s = s->next) {
		if (s->disabled != 2 || s->mode != WPAS_MODE_P2P_GO)
			continue;

		if (s->ssid_len == ssid->ssid_len &&
		    os_memcmp(s->ssid, ssid->ssid, s->ssid_len) == 0)
			break;
	}

	if (s == NULL)
		return;

	for (i = 0; s->p2p_client_list && i < s->num_p2p_clients; i++) {
		if (os_memcmp(s->p2p_client_list + i * ETH_ALEN, addr,
			      ETH_ALEN) != 0)
			continue;

		if (i == s->num_p2p_clients - 1)
			return; /* already the most recent entry */

		/* move the entry to mark it most recent */
		os_memmove(s->p2p_client_list + i * ETH_ALEN,
			   s->p2p_client_list + (i + 1) * ETH_ALEN,
			   (s->num_p2p_clients - i - 1) * ETH_ALEN);
		os_memcpy(s->p2p_client_list +
			  (s->num_p2p_clients - 1) * ETH_ALEN, addr, ETH_ALEN);
		found = 1;
		break;
	}

	if (!found && s->num_p2p_clients < P2P_MAX_STORED_CLIENTS) {
		n = os_realloc_array(s->p2p_client_list,
				     s->num_p2p_clients + 1, ETH_ALEN);
		if (n == NULL)
			return;
		os_memcpy(n + s->num_p2p_clients * ETH_ALEN, addr, ETH_ALEN);
		s->p2p_client_list = n;
		s->num_p2p_clients++;
	} else if (!found && s->p2p_client_list) {
		/* Not enough room for an additional entry - drop the oldest
		 * entry */
		os_memmove(s->p2p_client_list,
			   s->p2p_client_list + ETH_ALEN,
			   (s->num_p2p_clients - 1) * ETH_ALEN);
		os_memcpy(s->p2p_client_list +
			  (s->num_p2p_clients - 1) * ETH_ALEN,
			  addr, ETH_ALEN);
	}

	if (wpa_s->parent->conf->update_config &&
	    wpa_config_write(wpa_s->parent->confname, wpa_s->parent->conf))
		wpa_printf(MSG_DEBUG, "P2P: Failed to update configuration");
}


static void wpas_p2p_group_started(struct wpa_supplicant *wpa_s,
				   int go, struct wpa_ssid *ssid, int freq,
				   const u8 *psk, const char *passphrase,
				   const u8 *go_dev_addr, int persistent,
				   const char *extra)
{
	const char *ssid_txt;
	char psk_txt[65];

	if (psk)
		wpa_snprintf_hex(psk_txt, sizeof(psk_txt), psk, 32);
	else
		psk_txt[0] = '\0';

	if (ssid)
		ssid_txt = wpa_ssid_txt(ssid->ssid, ssid->ssid_len);
	else
		ssid_txt = "";

	if (passphrase && passphrase[0] == '\0')
		passphrase = NULL;

	/*
	 * Include PSK/passphrase only in the control interface message and
	 * leave it out from the debug log entry.
	 */
	wpa_msg_global_ctrl(wpa_s->parent, MSG_INFO,
			    P2P_EVENT_GROUP_STARTED
			    "%s %s ssid=\"%s\" freq=%d%s%s%s%s%s go_dev_addr="
			    MACSTR "%s%s",
			    wpa_s->ifname, go ? "GO" : "client", ssid_txt, freq,
			    psk ? " psk=" : "", psk_txt,
			    passphrase ? " passphrase=\"" : "",
			    passphrase ? passphrase : "",
			    passphrase ? "\"" : "",
			    MAC2STR(go_dev_addr),
			    persistent ? " [PERSISTENT]" : "", extra);
	wpa_printf(MSG_INFO, P2P_EVENT_GROUP_STARTED
		   "%s %s ssid=\"%s\" freq=%d go_dev_addr=" MACSTR "%s%s",
		   wpa_s->ifname, go ? "GO" : "client", ssid_txt, freq,
		   MAC2STR(go_dev_addr), persistent ? " [PERSISTENT]" : "",
		   extra);
}


static void wpas_group_formation_completed(struct wpa_supplicant *wpa_s,
					   int success)
{
	struct wpa_ssid *ssid;
	int client;
	int persistent;
	u8 go_dev_addr[ETH_ALEN];
	int network_id = -1;

	/*
	 * This callback is likely called for the main interface. Update wpa_s
	 * to use the group interface if a new interface was created for the
	 * group.
	 */
	if (wpa_s->global->p2p_group_formation)
		wpa_s = wpa_s->global->p2p_group_formation;
	if (wpa_s->p2p_go_group_formation_completed) {
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
	}
	wpa_s->p2p_in_invitation = 0;

	if (!success) {
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_GROUP_FORMATION_FAILURE);
		wpas_p2p_group_delete(wpa_s,
				      P2P_GROUP_REMOVAL_FORMATION_FAILED);
		return;
	}

	wpa_msg_global(wpa_s->parent, MSG_INFO,
		       P2P_EVENT_GROUP_FORMATION_SUCCESS);

	ssid = wpa_s->current_ssid;
	if (ssid && ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION) {
		ssid->mode = WPAS_MODE_P2P_GO;
		p2p_group_notif_formation_done(wpa_s->p2p_group);
		wpa_supplicant_ap_mac_addr_filter(wpa_s, NULL);
	}

	persistent = 0;
	if (ssid) {
		client = ssid->mode == WPAS_MODE_INFRA;
		if (ssid->mode == WPAS_MODE_P2P_GO) {
			persistent = ssid->p2p_persistent_group;
			os_memcpy(go_dev_addr, wpa_s->global->p2p_dev_addr,
				  ETH_ALEN);
		} else
			persistent = wpas_p2p_persistent_group(wpa_s,
							       go_dev_addr,
							       ssid->ssid,
							       ssid->ssid_len);
	} else {
		client = wpa_s->p2p_group_interface ==
			P2P_GROUP_INTERFACE_CLIENT;
		os_memset(go_dev_addr, 0, ETH_ALEN);
	}

	wpa_s->show_group_started = 0;
	if (client) {
		/*
		 * Indicate event only after successfully completed 4-way
		 * handshake, i.e., when the interface is ready for data
		 * packets.
		 */
		wpa_s->show_group_started = 1;
	} else {
		wpas_p2p_group_started(wpa_s, 1, ssid,
				       ssid ? ssid->frequency : 0,
				       ssid && ssid->passphrase == NULL &&
				       ssid->psk_set ? ssid->psk : NULL,
				       ssid ? ssid->passphrase : NULL,
				       go_dev_addr, persistent, "");
		wpas_p2p_cross_connect_setup(wpa_s);
		wpas_p2p_set_group_idle_timeout(wpa_s);
	}

	if (persistent)
		network_id = wpas_p2p_store_persistent_group(wpa_s->parent,
							     ssid, go_dev_addr);
	else {
		os_free(wpa_s->global->add_psk);
		wpa_s->global->add_psk = NULL;
	}
	if (network_id < 0 && ssid)
		network_id = ssid->id;
	if (!client) {
		wpas_notify_p2p_group_started(wpa_s, ssid, network_id, 0);
		os_get_reltime(&wpa_s->global->p2p_go_wait_client);
	}
}


struct send_action_work {
	unsigned int freq;
	u8 dst[ETH_ALEN];
	u8 src[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	size_t len;
	unsigned int wait_time;
	u8 buf[0];
};


static void wpas_p2p_send_action_work_timeout(void *eloop_ctx,
					      void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (!wpa_s->p2p_send_action_work)
		return;

	wpa_printf(MSG_DEBUG, "P2P: Send Action frame radio work timed out");
	os_free(wpa_s->p2p_send_action_work->ctx);
	radio_work_done(wpa_s->p2p_send_action_work);
	wpa_s->p2p_send_action_work = NULL;
}


static void wpas_p2p_action_tx_clear(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->p2p_send_action_work) {
		struct send_action_work *awork;
		awork = wpa_s->p2p_send_action_work->ctx;
		if (awork->wait_time == 0) {
			os_free(awork);
			radio_work_done(wpa_s->p2p_send_action_work);
			wpa_s->p2p_send_action_work = NULL;
		} else {
			/*
			 * In theory, this should not be needed, but number of
			 * places in the P2P code is still using non-zero wait
			 * time for the last Action frame in the sequence and
			 * some of these do not call send_action_done().
			 */
			eloop_cancel_timeout(wpas_p2p_send_action_work_timeout,
					     wpa_s, NULL);
			eloop_register_timeout(
				0, awork->wait_time * 1000,
				wpas_p2p_send_action_work_timeout,
				wpa_s, NULL);
		}
	}
}


static void wpas_p2p_send_action_tx_status(struct wpa_supplicant *wpa_s,
					   unsigned int freq,
					   const u8 *dst, const u8 *src,
					   const u8 *bssid,
					   const u8 *data, size_t data_len,
					   enum offchannel_send_action_result
					   result)
{
	enum p2p_send_action_result res = P2P_SEND_ACTION_SUCCESS;

	wpas_p2p_action_tx_clear(wpa_s);

	if (wpa_s->global->p2p == NULL || wpa_s->global->p2p_disabled)
		return;

	switch (result) {
	case OFFCHANNEL_SEND_ACTION_SUCCESS:
		res = P2P_SEND_ACTION_SUCCESS;
		break;
	case OFFCHANNEL_SEND_ACTION_NO_ACK:
		res = P2P_SEND_ACTION_NO_ACK;
		break;
	case OFFCHANNEL_SEND_ACTION_FAILED:
		res = P2P_SEND_ACTION_FAILED;
		break;
	}

	p2p_send_action_cb(wpa_s->global->p2p, freq, dst, src, bssid, res);

	if (result != OFFCHANNEL_SEND_ACTION_SUCCESS &&
	    wpa_s->pending_pd_before_join &&
	    (os_memcmp(dst, wpa_s->pending_join_dev_addr, ETH_ALEN) == 0 ||
	     os_memcmp(dst, wpa_s->pending_join_iface_addr, ETH_ALEN) == 0) &&
	    wpa_s->p2p_fallback_to_go_neg) {
		wpa_s->pending_pd_before_join = 0;
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: No ACK for PD Req "
			"during p2p_connect-auto");
		wpas_p2p_fallback_to_go_neg(wpa_s, 0);
		return;
	}
}


static void wpas_send_action_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct send_action_work *awork = work->ctx;

	if (deinit) {
		if (work->started) {
			eloop_cancel_timeout(wpas_p2p_send_action_work_timeout,
					     wpa_s, NULL);
			wpa_s->p2p_send_action_work = NULL;
			offchannel_send_action_done(wpa_s);
		}
		os_free(awork);
		return;
	}

	if (offchannel_send_action(wpa_s, awork->freq, awork->dst, awork->src,
				   awork->bssid, awork->buf, awork->len,
				   awork->wait_time,
				   wpas_p2p_send_action_tx_status, 1) < 0) {
		os_free(awork);
		radio_work_done(work);
		return;
	}
	wpa_s->p2p_send_action_work = work;
}


static int wpas_send_action_work(struct wpa_supplicant *wpa_s,
				 unsigned int freq, const u8 *dst,
				 const u8 *src, const u8 *bssid, const u8 *buf,
				 size_t len, unsigned int wait_time)
{
	struct send_action_work *awork;

	if (wpa_s->p2p_send_action_work) {
		wpa_printf(MSG_DEBUG, "P2P: Cannot schedule new p2p-send-action work since one is already pending");
		return -1;
	}

	awork = os_zalloc(sizeof(*awork) + len);
	if (awork == NULL)
		return -1;

	awork->freq = freq;
	os_memcpy(awork->dst, dst, ETH_ALEN);
	os_memcpy(awork->src, src, ETH_ALEN);
	os_memcpy(awork->bssid, bssid, ETH_ALEN);
	awork->len = len;
	awork->wait_time = wait_time;
	os_memcpy(awork->buf, buf, len);

	if (radio_add_work(wpa_s, freq, "p2p-send-action", 0,
			   wpas_send_action_cb, awork) < 0) {
		os_free(awork);
		return -1;
	}

	return 0;
}


static int wpas_send_action(void *ctx, unsigned int freq, const u8 *dst,
			    const u8 *src, const u8 *bssid, const u8 *buf,
			    size_t len, unsigned int wait_time)
{
	struct wpa_supplicant *wpa_s = ctx;
	int listen_freq = -1, send_freq = -1;

	if (wpa_s->p2p_listen_work)
		listen_freq = wpa_s->p2p_listen_work->freq;
	if (wpa_s->p2p_send_action_work)
		send_freq = wpa_s->p2p_send_action_work->freq;
	if (listen_freq != (int) freq && send_freq != (int) freq) {
		wpa_printf(MSG_DEBUG, "P2P: Schedule new radio work for Action frame TX (listen_freq=%d send_freq=%d)",
			   listen_freq, send_freq);
		return wpas_send_action_work(wpa_s, freq, dst, src, bssid, buf,
					     len, wait_time);
	}

	wpa_printf(MSG_DEBUG, "P2P: Use ongoing radio work for Action frame TX");
	return offchannel_send_action(wpa_s, freq, dst, src, bssid, buf, len,
				      wait_time,
				      wpas_p2p_send_action_tx_status, 1);
}


static void wpas_send_action_done(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->p2p_send_action_work) {
		eloop_cancel_timeout(wpas_p2p_send_action_work_timeout,
				     wpa_s, NULL);
		os_free(wpa_s->p2p_send_action_work->ctx);
		radio_work_done(wpa_s->p2p_send_action_work);
		wpa_s->p2p_send_action_work = NULL;
	}

	offchannel_send_action_done(wpa_s);
}


static int wpas_copy_go_neg_results(struct wpa_supplicant *wpa_s,
				    struct p2p_go_neg_results *params)
{
	if (wpa_s->go_params == NULL) {
		wpa_s->go_params = os_malloc(sizeof(*params));
		if (wpa_s->go_params == NULL)
			return -1;
	}
	os_memcpy(wpa_s->go_params, params, sizeof(*params));
	return 0;
}


static void wpas_start_wps_enrollee(struct wpa_supplicant *wpa_s,
				    struct p2p_go_neg_results *res)
{
	wpa_printf(MSG_DEBUG, "P2P: Start WPS Enrollee for peer " MACSTR
		   " dev_addr " MACSTR " wps_method %d",
		   MAC2STR(res->peer_interface_addr),
		   MAC2STR(res->peer_device_addr), res->wps_method);
	wpa_hexdump_ascii(MSG_DEBUG, "P2P: Start WPS Enrollee for SSID",
			  res->ssid, res->ssid_len);
	wpa_supplicant_ap_deinit(wpa_s);
	wpas_copy_go_neg_results(wpa_s, res);
	if (res->wps_method == WPS_PBC) {
		wpas_wps_start_pbc(wpa_s, res->peer_interface_addr, 1);
#ifdef CONFIG_WPS_NFC
	} else if (res->wps_method == WPS_NFC) {
		wpas_wps_start_nfc(wpa_s, res->peer_device_addr,
				   res->peer_interface_addr,
				   wpa_s->parent->p2p_oob_dev_pw,
				   wpa_s->parent->p2p_oob_dev_pw_id, 1,
				   wpa_s->parent->p2p_oob_dev_pw_id ==
				   DEV_PW_NFC_CONNECTION_HANDOVER ?
				   wpa_s->parent->p2p_peer_oob_pubkey_hash :
				   NULL,
				   NULL, 0, 0);
#endif /* CONFIG_WPS_NFC */
	} else {
		u16 dev_pw_id = DEV_PW_DEFAULT;
		if (wpa_s->p2p_wps_method == WPS_PIN_KEYPAD)
			dev_pw_id = DEV_PW_REGISTRAR_SPECIFIED;
		wpas_wps_start_pin(wpa_s, res->peer_interface_addr,
				   wpa_s->p2p_pin, 1, dev_pw_id);
	}
}


static void wpas_p2p_add_psk_list(struct wpa_supplicant *wpa_s,
				  struct wpa_ssid *ssid)
{
	struct wpa_ssid *persistent;
	struct psk_list_entry *psk;
	struct hostapd_data *hapd;

	if (!wpa_s->ap_iface)
		return;

	persistent = wpas_p2p_get_persistent(wpa_s->parent, NULL, ssid->ssid,
					     ssid->ssid_len);
	if (persistent == NULL)
		return;

	hapd = wpa_s->ap_iface->bss[0];

	dl_list_for_each(psk, &persistent->psk_list, struct psk_list_entry,
			 list) {
		struct hostapd_wpa_psk *hpsk;

		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Add persistent group PSK entry for "
			MACSTR " psk=%d",
			MAC2STR(psk->addr), psk->p2p);
		hpsk = os_zalloc(sizeof(*hpsk));
		if (hpsk == NULL)
			break;
		os_memcpy(hpsk->psk, psk->psk, PMK_LEN);
		if (psk->p2p)
			os_memcpy(hpsk->p2p_dev_addr, psk->addr, ETH_ALEN);
		else
			os_memcpy(hpsk->addr, psk->addr, ETH_ALEN);
		hpsk->next = hapd->conf->ssid.wpa_psk;
		hapd->conf->ssid.wpa_psk = hpsk;
	}
}


static void p2p_go_configured(void *ctx, void *data)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct p2p_go_neg_results *params = data;
	struct wpa_ssid *ssid;
	int network_id = -1;

	ssid = wpa_s->current_ssid;
	if (ssid && ssid->mode == WPAS_MODE_P2P_GO) {
		wpa_printf(MSG_DEBUG, "P2P: Group setup without provisioning");
		if (wpa_s->global->p2p_group_formation == wpa_s)
			wpa_s->global->p2p_group_formation = NULL;
		wpas_p2p_group_started(wpa_s, 1, ssid, ssid->frequency,
				       params->passphrase[0] == '\0' ?
				       params->psk : NULL,
				       params->passphrase,
				       wpa_s->global->p2p_dev_addr,
				       params->persistent_group, "");

		os_get_reltime(&wpa_s->global->p2p_go_wait_client);
		if (params->persistent_group) {
			network_id = wpas_p2p_store_persistent_group(
				wpa_s->parent, ssid,
				wpa_s->global->p2p_dev_addr);
			wpas_p2p_add_psk_list(wpa_s, ssid);
		}
		if (network_id < 0)
			network_id = ssid->id;
		wpas_notify_p2p_group_started(wpa_s, ssid, network_id, 0);
		wpas_p2p_cross_connect_setup(wpa_s);
		wpas_p2p_set_group_idle_timeout(wpa_s);

		if (wpa_s->p2p_first_connection_timeout) {
			wpa_dbg(wpa_s, MSG_DEBUG,
				"P2P: Start group formation timeout of %d seconds until first data connection on GO",
				wpa_s->p2p_first_connection_timeout);
			wpa_s->p2p_go_group_formation_completed = 0;
			wpa_s->global->p2p_group_formation = wpa_s;
			eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
					     wpa_s->parent, NULL);
			eloop_register_timeout(
				wpa_s->p2p_first_connection_timeout, 0,
				wpas_p2p_group_formation_timeout,
				wpa_s->parent, NULL);
		}

		return;
	}

	wpa_printf(MSG_DEBUG, "P2P: Setting up WPS for GO provisioning");
	if (wpa_supplicant_ap_mac_addr_filter(wpa_s,
					      params->peer_interface_addr)) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to setup MAC address "
			   "filtering");
		return;
	}
	if (params->wps_method == WPS_PBC) {
		wpa_supplicant_ap_wps_pbc(wpa_s, params->peer_interface_addr,
					  params->peer_device_addr);
#ifdef CONFIG_WPS_NFC
	} else if (params->wps_method == WPS_NFC) {
		if (wpa_s->parent->p2p_oob_dev_pw_id !=
		    DEV_PW_NFC_CONNECTION_HANDOVER &&
		    !wpa_s->parent->p2p_oob_dev_pw) {
			wpa_printf(MSG_DEBUG, "P2P: No NFC Dev Pw known");
			return;
		}
		wpas_ap_wps_add_nfc_pw(
			wpa_s, wpa_s->parent->p2p_oob_dev_pw_id,
			wpa_s->parent->p2p_oob_dev_pw,
			wpa_s->parent->p2p_peer_oob_pk_hash_known ?
			wpa_s->parent->p2p_peer_oob_pubkey_hash : NULL);
#endif /* CONFIG_WPS_NFC */
	} else if (wpa_s->p2p_pin[0])
		wpa_supplicant_ap_wps_pin(wpa_s, params->peer_interface_addr,
					  wpa_s->p2p_pin, NULL, 0, 0);
	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;
}


static void wpas_start_wps_go(struct wpa_supplicant *wpa_s,
			      struct p2p_go_neg_results *params,
			      int group_formation)
{
	struct wpa_ssid *ssid;

	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Starting GO");
	if (wpas_copy_go_neg_results(wpa_s, params) < 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Could not copy GO Negotiation "
			"results");
		return;
	}

	ssid = wpa_config_add_network(wpa_s->conf);
	if (ssid == NULL) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Could not add network for GO");
		return;
	}

	wpa_s->show_group_started = 0;

	wpa_config_set_network_defaults(ssid);
	ssid->temporary = 1;
	ssid->p2p_group = 1;
	ssid->p2p_persistent_group = params->persistent_group;
	ssid->mode = group_formation ? WPAS_MODE_P2P_GROUP_FORMATION :
		WPAS_MODE_P2P_GO;
	ssid->frequency = params->freq;
	ssid->ht40 = params->ht40;
	ssid->vht = params->vht;
	ssid->ssid = os_zalloc(params->ssid_len + 1);
	if (ssid->ssid) {
		os_memcpy(ssid->ssid, params->ssid, params->ssid_len);
		ssid->ssid_len = params->ssid_len;
	}
	ssid->auth_alg = WPA_AUTH_ALG_OPEN;
	ssid->key_mgmt = WPA_KEY_MGMT_PSK;
	ssid->proto = WPA_PROTO_RSN;
	ssid->pairwise_cipher = WPA_CIPHER_CCMP;
	if (os_strlen(params->passphrase) > 0) {
		ssid->passphrase = os_strdup(params->passphrase);
		if (ssid->passphrase == NULL) {
			wpa_msg_global(wpa_s, MSG_ERROR,
				       "P2P: Failed to copy passphrase for GO");
			wpa_config_remove_network(wpa_s->conf, ssid->id);
			return;
		}
	} else
		ssid->passphrase = NULL;
	ssid->psk_set = params->psk_set;
	if (ssid->psk_set)
		os_memcpy(ssid->psk, params->psk, sizeof(ssid->psk));
	else if (ssid->passphrase)
		wpa_config_update_psk(ssid);
	ssid->ap_max_inactivity = wpa_s->parent->conf->p2p_go_max_inactivity;

	wpa_s->ap_configured_cb = p2p_go_configured;
	wpa_s->ap_configured_cb_ctx = wpa_s;
	wpa_s->ap_configured_cb_data = wpa_s->go_params;
	wpa_s->scan_req = NORMAL_SCAN_REQ;
	wpa_s->connect_without_scan = ssid;
	wpa_s->reassociate = 1;
	wpa_s->disconnected = 0;
	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Request scan (that will be skipped) to "
		"start GO)");
	wpa_supplicant_req_scan(wpa_s, 0, 0);
}


static void wpas_p2p_clone_config(struct wpa_supplicant *dst,
				  const struct wpa_supplicant *src)
{
	struct wpa_config *d;
	const struct wpa_config *s;

	d = dst->conf;
	s = src->conf;

#define C(n) if (s->n) d->n = os_strdup(s->n)
	C(device_name);
	C(manufacturer);
	C(model_name);
	C(model_number);
	C(serial_number);
	C(config_methods);
#undef C

	os_memcpy(d->device_type, s->device_type, WPS_DEV_TYPE_LEN);
	os_memcpy(d->sec_device_type, s->sec_device_type,
		  sizeof(d->sec_device_type));
	d->num_sec_device_types = s->num_sec_device_types;

	d->p2p_group_idle = s->p2p_group_idle;
	d->p2p_intra_bss = s->p2p_intra_bss;
	d->persistent_reconnect = s->persistent_reconnect;
	d->max_num_sta = s->max_num_sta;
	d->pbc_in_m1 = s->pbc_in_m1;
	d->ignore_old_scan_res = s->ignore_old_scan_res;
	d->beacon_int = s->beacon_int;
	d->dtim_period = s->dtim_period;
	d->disassoc_low_ack = s->disassoc_low_ack;
	d->disable_scan_offload = s->disable_scan_offload;

	if (s->wps_nfc_dh_privkey && s->wps_nfc_dh_pubkey) {
		d->wps_nfc_dh_privkey = wpabuf_dup(s->wps_nfc_dh_privkey);
		d->wps_nfc_dh_pubkey = wpabuf_dup(s->wps_nfc_dh_pubkey);
	}
}


static void wpas_p2p_get_group_ifname(struct wpa_supplicant *wpa_s,
				      char *ifname, size_t len)
{
	char *ifname_ptr = wpa_s->ifname;

	if (os_strncmp(wpa_s->ifname, P2P_MGMT_DEVICE_PREFIX,
		       os_strlen(P2P_MGMT_DEVICE_PREFIX)) == 0) {
		ifname_ptr = os_strrchr(wpa_s->ifname, '-') + 1;
	}

	os_snprintf(ifname, len, "p2p-%s-%d", ifname_ptr, wpa_s->p2p_group_idx);
	if (os_strlen(ifname) >= IFNAMSIZ &&
	    os_strlen(wpa_s->ifname) < IFNAMSIZ) {
		/* Try to avoid going over the IFNAMSIZ length limit */
		os_snprintf(ifname, len, "p2p-%d", wpa_s->p2p_group_idx);
	}
}


static int wpas_p2p_add_group_interface(struct wpa_supplicant *wpa_s,
					enum wpa_driver_if_type type)
{
	char ifname[120], force_ifname[120];

	if (wpa_s->pending_interface_name[0]) {
		wpa_printf(MSG_DEBUG, "P2P: Pending virtual interface exists "
			   "- skip creation of a new one");
		if (is_zero_ether_addr(wpa_s->pending_interface_addr)) {
			wpa_printf(MSG_DEBUG, "P2P: Pending virtual address "
				   "unknown?! ifname='%s'",
				   wpa_s->pending_interface_name);
			return -1;
		}
		return 0;
	}

	wpas_p2p_get_group_ifname(wpa_s, ifname, sizeof(ifname));
	force_ifname[0] = '\0';

	wpa_printf(MSG_DEBUG, "P2P: Create a new interface %s for the group",
		   ifname);
	wpa_s->p2p_group_idx++;

	wpa_s->pending_interface_type = type;
	if (wpa_drv_if_add(wpa_s, type, ifname, NULL, NULL, force_ifname,
			   wpa_s->pending_interface_addr, NULL) < 0) {
		wpa_printf(MSG_ERROR, "P2P: Failed to create new group "
			   "interface");
		return -1;
	}

	if (force_ifname[0]) {
		wpa_printf(MSG_DEBUG, "P2P: Driver forced interface name %s",
			   force_ifname);
		os_strlcpy(wpa_s->pending_interface_name, force_ifname,
			   sizeof(wpa_s->pending_interface_name));
	} else
		os_strlcpy(wpa_s->pending_interface_name, ifname,
			   sizeof(wpa_s->pending_interface_name));
	wpa_printf(MSG_DEBUG, "P2P: Created pending virtual interface %s addr "
		   MACSTR, wpa_s->pending_interface_name,
		   MAC2STR(wpa_s->pending_interface_addr));

	return 0;
}


static void wpas_p2p_remove_pending_group_interface(
	struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->pending_interface_name[0] ||
	    is_zero_ether_addr(wpa_s->pending_interface_addr))
		return; /* No pending virtual interface */

	wpa_printf(MSG_DEBUG, "P2P: Removing pending group interface %s",
		   wpa_s->pending_interface_name);
	wpa_drv_if_remove(wpa_s, wpa_s->pending_interface_type,
			  wpa_s->pending_interface_name);
	os_memset(wpa_s->pending_interface_addr, 0, ETH_ALEN);
	wpa_s->pending_interface_name[0] = '\0';
}


static struct wpa_supplicant *
wpas_p2p_init_group_interface(struct wpa_supplicant *wpa_s, int go)
{
	struct wpa_interface iface;
	struct wpa_supplicant *group_wpa_s;

	if (!wpa_s->pending_interface_name[0]) {
		wpa_printf(MSG_ERROR, "P2P: No pending group interface");
		if (!wpas_p2p_create_iface(wpa_s))
			return NULL;
		/*
		 * Something has forced us to remove the pending interface; try
		 * to create a new one and hope for the best that we will get
		 * the same local address.
		 */
		if (wpas_p2p_add_group_interface(wpa_s, go ? WPA_IF_P2P_GO :
						 WPA_IF_P2P_CLIENT) < 0)
			return NULL;
	}

	os_memset(&iface, 0, sizeof(iface));
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;
	if (wpa_s->conf->ctrl_interface == NULL &&
	    wpa_s->parent != wpa_s &&
	    wpa_s->p2p_mgmt &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE))
		iface.ctrl_interface = wpa_s->parent->conf->ctrl_interface;
	else
		iface.ctrl_interface = wpa_s->conf->ctrl_interface;
	iface.driver_param = wpa_s->conf->driver_param;
	group_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface);
	if (group_wpa_s == NULL) {
		wpa_printf(MSG_ERROR, "P2P: Failed to create new "
			   "wpa_supplicant interface");
		return NULL;
	}
	wpa_s->pending_interface_name[0] = '\0';
	group_wpa_s->parent = wpa_s;
	group_wpa_s->p2p_group_interface = go ? P2P_GROUP_INTERFACE_GO :
		P2P_GROUP_INTERFACE_CLIENT;
	wpa_s->global->p2p_group_formation = group_wpa_s;

	wpas_p2p_clone_config(group_wpa_s, wpa_s);

	return group_wpa_s;
}


static void wpas_p2p_group_formation_timeout(void *eloop_ctx,
					     void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpa_printf(MSG_DEBUG, "P2P: Group Formation timed out");
	wpas_p2p_group_formation_failed(wpa_s);
}


void wpas_p2p_group_formation_failed(struct wpa_supplicant *wpa_s)
{
	eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
			     wpa_s->parent, NULL);
	if (wpa_s->global->p2p)
		p2p_group_formation_failed(wpa_s->global->p2p);
	wpas_group_formation_completed(wpa_s, 0);
}


static void wpas_p2p_grpform_fail_after_wps(struct wpa_supplicant *wpa_s)
{
	wpa_printf(MSG_DEBUG, "P2P: Reject group formation due to WPS provisioning failure");
	eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
			     wpa_s->parent, NULL);
	eloop_register_timeout(0, 0, wpas_p2p_group_formation_timeout,
			       wpa_s->parent, NULL);
	wpa_s->global->p2p_fail_on_wps_complete = 0;
}


void wpas_p2p_ap_setup_failed(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->global->p2p_group_formation != wpa_s)
		return;
	/* Speed up group formation timeout since this cannot succeed */
	eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
			     wpa_s->parent, NULL);
	eloop_register_timeout(0, 0, wpas_p2p_group_formation_timeout,
			       wpa_s->parent, NULL);
}


static void wpas_go_neg_completed(void *ctx, struct p2p_go_neg_results *res)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}

	if (res->status) {
		wpa_msg_global(wpa_s, MSG_INFO,
			       P2P_EVENT_GO_NEG_FAILURE "status=%d",
			       res->status);
		wpas_notify_p2p_go_neg_completed(wpa_s, res);
		wpas_p2p_remove_pending_group_interface(wpa_s);
		return;
	}

	if (wpa_s->p2p_go_ht40)
		res->ht40 = 1;
	if (wpa_s->p2p_go_vht)
		res->vht = 1;

	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_GO_NEG_SUCCESS "role=%s "
		       "freq=%d ht40=%d peer_dev=" MACSTR " peer_iface=" MACSTR
		       " wps_method=%s",
		       res->role_go ? "GO" : "client", res->freq, res->ht40,
		       MAC2STR(res->peer_device_addr),
		       MAC2STR(res->peer_interface_addr),
		       p2p_wps_method_text(res->wps_method));
	wpas_notify_p2p_go_neg_completed(wpa_s, res);

	if (res->role_go && wpa_s->p2p_persistent_id >= 0) {
		struct wpa_ssid *ssid;
		ssid = wpa_config_get_network(wpa_s->conf,
					      wpa_s->p2p_persistent_id);
		if (ssid && ssid->disabled == 2 &&
		    ssid->mode == WPAS_MODE_P2P_GO && ssid->passphrase) {
			size_t len = os_strlen(ssid->passphrase);
			wpa_printf(MSG_DEBUG, "P2P: Override passphrase based "
				   "on requested persistent group");
			os_memcpy(res->passphrase, ssid->passphrase, len);
			res->passphrase[len] = '\0';
		}
	}

	if (wpa_s->create_p2p_iface) {
		struct wpa_supplicant *group_wpa_s =
			wpas_p2p_init_group_interface(wpa_s, res->role_go);
		if (group_wpa_s == NULL) {
			wpas_p2p_remove_pending_group_interface(wpa_s);
			eloop_cancel_timeout(wpas_p2p_long_listen_timeout,
					     wpa_s, NULL);
			wpas_p2p_group_formation_failed(wpa_s);
			return;
		}
		if (group_wpa_s != wpa_s) {
			os_memcpy(group_wpa_s->p2p_pin, wpa_s->p2p_pin,
				  sizeof(group_wpa_s->p2p_pin));
			group_wpa_s->p2p_wps_method = wpa_s->p2p_wps_method;
		}
		os_memset(wpa_s->pending_interface_addr, 0, ETH_ALEN);
		wpa_s->pending_interface_name[0] = '\0';
		group_wpa_s->p2p_in_provisioning = 1;

		if (res->role_go)
			wpas_start_wps_go(group_wpa_s, res, 1);
		else
			wpas_start_wps_enrollee(group_wpa_s, res);
	} else {
		wpa_s->p2p_in_provisioning = 1;
		wpa_s->global->p2p_group_formation = wpa_s;

		if (res->role_go)
			wpas_start_wps_go(wpa_s, res, 1);
		else
			wpas_start_wps_enrollee(ctx, res);
	}

	wpa_s->p2p_long_listen = 0;
	eloop_cancel_timeout(wpas_p2p_long_listen_timeout, wpa_s, NULL);

	eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s, NULL);
	eloop_register_timeout(15 + res->peer_config_timeout / 100,
			       (res->peer_config_timeout % 100) * 10000,
			       wpas_p2p_group_formation_timeout, wpa_s, NULL);
}


static void wpas_go_neg_req_rx(void *ctx, const u8 *src, u16 dev_passwd_id)
{
	struct wpa_supplicant *wpa_s = ctx;
	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_GO_NEG_REQUEST MACSTR
		       " dev_passwd_id=%u", MAC2STR(src), dev_passwd_id);

	wpas_notify_p2p_go_neg_req(wpa_s, src, dev_passwd_id);
}


static void wpas_dev_found(void *ctx, const u8 *addr,
			   const struct p2p_peer_info *info,
			   int new_device)
{
#ifndef CONFIG_NO_STDOUT_DEBUG
	struct wpa_supplicant *wpa_s = ctx;
	char devtype[WPS_DEV_TYPE_BUFSIZE];
	char *wfd_dev_info_hex = NULL;

#ifdef CONFIG_WIFI_DISPLAY
	wfd_dev_info_hex = wifi_display_subelem_hex(info->wfd_subelems,
						    WFD_SUBELEM_DEVICE_INFO);
#endif /* CONFIG_WIFI_DISPLAY */

	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_DEVICE_FOUND MACSTR
		       " p2p_dev_addr=" MACSTR
		       " pri_dev_type=%s name='%s' config_methods=0x%x "
		       "dev_capab=0x%x group_capab=0x%x%s%s%s",
		       MAC2STR(addr), MAC2STR(info->p2p_device_addr),
		       wps_dev_type_bin2str(info->pri_dev_type, devtype,
					    sizeof(devtype)),
		       info->device_name, info->config_methods,
		       info->dev_capab, info->group_capab,
		       wfd_dev_info_hex ? " wfd_dev_info=0x" : "",
		       wfd_dev_info_hex ? wfd_dev_info_hex : "",
		       info->vendor_elems ? " vendor_elems=1" : "");

	os_free(wfd_dev_info_hex);
#endif /* CONFIG_NO_STDOUT_DEBUG */

	wpas_notify_p2p_device_found(ctx, info->p2p_device_addr, new_device);
}


static void wpas_dev_lost(void *ctx, const u8 *dev_addr)
{
	struct wpa_supplicant *wpa_s = ctx;

	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_DEVICE_LOST
		       "p2p_dev_addr=" MACSTR, MAC2STR(dev_addr));

	wpas_notify_p2p_device_lost(wpa_s, dev_addr);
}


static void wpas_find_stopped(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_FIND_STOPPED);
}


struct wpas_p2p_listen_work {
	unsigned int freq;
	unsigned int duration;
	struct wpabuf *probe_resp_ie;
};


static void wpas_p2p_listen_work_free(struct wpas_p2p_listen_work *lwork)
{
	if (lwork == NULL)
		return;
	wpabuf_free(lwork->probe_resp_ie);
	os_free(lwork);
}


static void wpas_p2p_listen_work_done(struct wpa_supplicant *wpa_s)
{
	struct wpas_p2p_listen_work *lwork;

	if (!wpa_s->p2p_listen_work)
		return;

	lwork = wpa_s->p2p_listen_work->ctx;
	wpas_p2p_listen_work_free(lwork);
	radio_work_done(wpa_s->p2p_listen_work);
	wpa_s->p2p_listen_work = NULL;
}


static void wpas_start_listen_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct wpas_p2p_listen_work *lwork = work->ctx;

	if (deinit) {
		if (work->started) {
			wpa_s->p2p_listen_work = NULL;
			wpas_stop_listen(wpa_s);
		}
		wpas_p2p_listen_work_free(lwork);
		return;
	}

	wpa_s->p2p_listen_work = work;

	wpa_drv_set_ap_wps_ie(wpa_s, NULL, lwork->probe_resp_ie, NULL);

	if (wpa_drv_probe_req_report(wpa_s, 1) < 0) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to request the driver to "
			   "report received Probe Request frames");
		wpas_p2p_listen_work_done(wpa_s);
		return;
	}

	wpa_s->pending_listen_freq = lwork->freq;
	wpa_s->pending_listen_duration = lwork->duration;

	if (wpa_drv_remain_on_channel(wpa_s, lwork->freq, lwork->duration) < 0)
	{
		wpa_printf(MSG_DEBUG, "P2P: Failed to request the driver "
			   "to remain on channel (%u MHz) for Listen "
			   "state", lwork->freq);
		wpas_p2p_listen_work_done(wpa_s);
		wpa_s->pending_listen_freq = 0;
		return;
	}
	wpa_s->off_channel_freq = 0;
	wpa_s->roc_waiting_drv_freq = lwork->freq;
}


static int wpas_start_listen(void *ctx, unsigned int freq,
			     unsigned int duration,
			     const struct wpabuf *probe_resp_ie)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpas_p2p_listen_work *lwork;

	if (wpa_s->p2p_listen_work) {
		wpa_printf(MSG_DEBUG, "P2P: Reject start_listen since p2p_listen_work already exists");
		return -1;
	}

	lwork = os_zalloc(sizeof(*lwork));
	if (lwork == NULL)
		return -1;
	lwork->freq = freq;
	lwork->duration = duration;
	if (probe_resp_ie) {
		lwork->probe_resp_ie = wpabuf_dup(probe_resp_ie);
		if (lwork->probe_resp_ie == NULL) {
			wpas_p2p_listen_work_free(lwork);
			return -1;
		}
	}

	if (radio_add_work(wpa_s, freq, "p2p-listen", 0, wpas_start_listen_cb,
			   lwork) < 0) {
		wpas_p2p_listen_work_free(lwork);
		return -1;
	}

	return 0;
}


static void wpas_stop_listen(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}
	wpa_drv_set_ap_wps_ie(wpa_s, NULL, NULL, NULL);
	wpa_drv_probe_req_report(wpa_s, 0);
	wpas_p2p_listen_work_done(wpa_s);
}


static int wpas_send_probe_resp(void *ctx, const struct wpabuf *buf)
{
	struct wpa_supplicant *wpa_s = ctx;
	return wpa_drv_send_mlme(wpa_s, wpabuf_head(buf), wpabuf_len(buf), 1);
}


/*
 * DNS Header section is used only to calculate compression pointers, so the
 * contents of this data does not matter, but the length needs to be reserved
 * in the virtual packet.
 */
#define DNS_HEADER_LEN 12

/*
 * 27-octet in-memory packet from P2P specification containing two implied
 * queries for _tcp.lcoal. PTR IN and _udp.local. PTR IN
 */
#define P2P_SD_IN_MEMORY_LEN 27

static int p2p_sd_dns_uncompress_label(char **upos, char *uend, u8 *start,
				       u8 **spos, const u8 *end)
{
	while (*spos < end) {
		u8 val = ((*spos)[0] & 0xc0) >> 6;
		int len;

		if (val == 1 || val == 2) {
			/* These are reserved values in RFC 1035 */
			wpa_printf(MSG_DEBUG, "P2P: Invalid domain name "
				   "sequence starting with 0x%x", val);
			return -1;
		}

		if (val == 3) {
			u16 offset;
			u8 *spos_tmp;

			/* Offset */
			if (*spos + 2 > end) {
				wpa_printf(MSG_DEBUG, "P2P: No room for full "
					   "DNS offset field");
				return -1;
			}

			offset = (((*spos)[0] & 0x3f) << 8) | (*spos)[1];
			if (offset >= *spos - start) {
				wpa_printf(MSG_DEBUG, "P2P: Invalid DNS "
					   "pointer offset %u", offset);
				return -1;
			}

			(*spos) += 2;
			spos_tmp = start + offset;
			return p2p_sd_dns_uncompress_label(upos, uend, start,
							   &spos_tmp,
							   *spos - 2);
		}

		/* Label */
		len = (*spos)[0] & 0x3f;
		if (len == 0)
			return 0;

		(*spos)++;
		if (*spos + len > end) {
			wpa_printf(MSG_DEBUG, "P2P: Invalid domain name "
				   "sequence - no room for label with length "
				   "%u", len);
			return -1;
		}

		if (*upos + len + 2 > uend)
			return -2;

		os_memcpy(*upos, *spos, len);
		*spos += len;
		*upos += len;
		(*upos)[0] = '.';
		(*upos)++;
		(*upos)[0] = '\0';
	}

	return 0;
}


/* Uncompress domain names per RFC 1035 using the P2P SD in-memory packet.
 * Returns -1 on parsing error (invalid input sequence), -2 if output buffer is
 * not large enough */
static int p2p_sd_dns_uncompress(char *buf, size_t buf_len, const u8 *msg,
				 size_t msg_len, size_t offset)
{
	/* 27-octet in-memory packet from P2P specification */
	const char *prefix = "\x04_tcp\x05local\x00\x00\x0C\x00\x01"
		"\x04_udp\xC0\x11\x00\x0C\x00\x01";
	u8 *tmp, *end, *spos;
	char *upos, *uend;
	int ret = 0;

	if (buf_len < 2)
		return -1;
	if (offset > msg_len)
		return -1;

	tmp = os_malloc(DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN + msg_len);
	if (tmp == NULL)
		return -1;
	spos = tmp + DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN;
	end = spos + msg_len;
	spos += offset;

	os_memset(tmp, 0, DNS_HEADER_LEN);
	os_memcpy(tmp + DNS_HEADER_LEN, prefix, P2P_SD_IN_MEMORY_LEN);
	os_memcpy(tmp + DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN, msg, msg_len);

	upos = buf;
	uend = buf + buf_len;

	ret = p2p_sd_dns_uncompress_label(&upos, uend, tmp, &spos, end);
	if (ret) {
		os_free(tmp);
		return ret;
	}

	if (upos == buf) {
		upos[0] = '.';
		upos[1] = '\0';
	} else if (upos[-1] == '.')
		upos[-1] = '\0';

	os_free(tmp);
	return 0;
}


static struct p2p_srv_bonjour *
wpas_p2p_service_get_bonjour(struct wpa_supplicant *wpa_s,
			     const struct wpabuf *query)
{
	struct p2p_srv_bonjour *bsrv;
	size_t len;

	len = wpabuf_len(query);
	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (len == wpabuf_len(bsrv->query) &&
		    os_memcmp(wpabuf_head(query), wpabuf_head(bsrv->query),
			      len) == 0)
			return bsrv;
	}
	return NULL;
}


static struct p2p_srv_upnp *
wpas_p2p_service_get_upnp(struct wpa_supplicant *wpa_s, u8 version,
			  const char *service)
{
	struct p2p_srv_upnp *usrv;

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (version == usrv->version &&
		    os_strcmp(service, usrv->service) == 0)
			return usrv;
	}
	return NULL;
}


static void wpas_sd_add_proto_not_avail(struct wpabuf *resp, u8 srv_proto,
					u8 srv_trans_id)
{
	u8 *len_pos;

	if (wpabuf_tailroom(resp) < 5)
		return;

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, srv_proto);
	wpabuf_put_u8(resp, srv_trans_id);
	/* Status Code */
	wpabuf_put_u8(resp, P2P_SD_PROTO_NOT_AVAILABLE);
	/* Response Data: empty */
	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}


static void wpas_sd_all_bonjour(struct wpa_supplicant *wpa_s,
				struct wpabuf *resp, u8 srv_trans_id)
{
	struct p2p_srv_bonjour *bsrv;
	u8 *len_pos;

	wpa_printf(MSG_DEBUG, "P2P: SD Request for all Bonjour services");

	if (dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
		wpa_printf(MSG_DEBUG, "P2P: Bonjour protocol not available");
		return;
	}

	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (wpabuf_tailroom(resp) <
		    5 + wpabuf_len(bsrv->query) + wpabuf_len(bsrv->resp))
			return;
		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);
		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);
		wpa_hexdump_ascii(MSG_DEBUG, "P2P: Matching Bonjour service",
				  wpabuf_head(bsrv->resp),
				  wpabuf_len(bsrv->resp));
		/* Response Data */
		wpabuf_put_buf(resp, bsrv->query); /* Key */
		wpabuf_put_buf(resp, bsrv->resp); /* Value */
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos -
			     2);
	}
}


static int match_bonjour_query(struct p2p_srv_bonjour *bsrv, const u8 *query,
			       size_t query_len)
{
	char str_rx[256], str_srv[256];

	if (query_len < 3 || wpabuf_len(bsrv->query) < 3)
		return 0; /* Too short to include DNS Type and Version */
	if (os_memcmp(query + query_len - 3,
		      wpabuf_head_u8(bsrv->query) + wpabuf_len(bsrv->query) - 3,
		      3) != 0)
		return 0; /* Mismatch in DNS Type or Version */
	if (query_len == wpabuf_len(bsrv->query) &&
	    os_memcmp(query, wpabuf_head(bsrv->query), query_len - 3) == 0)
		return 1; /* Binary match */

	if (p2p_sd_dns_uncompress(str_rx, sizeof(str_rx), query, query_len - 3,
				  0))
		return 0; /* Failed to uncompress query */
	if (p2p_sd_dns_uncompress(str_srv, sizeof(str_srv),
				  wpabuf_head(bsrv->query),
				  wpabuf_len(bsrv->query) - 3, 0))
		return 0; /* Failed to uncompress service */

	return os_strcmp(str_rx, str_srv) == 0;
}


static void wpas_sd_req_bonjour(struct wpa_supplicant *wpa_s,
				struct wpabuf *resp, u8 srv_trans_id,
				const u8 *query, size_t query_len)
{
	struct p2p_srv_bonjour *bsrv;
	u8 *len_pos;
	int matches = 0;

	wpa_hexdump_ascii(MSG_DEBUG, "P2P: SD Request for Bonjour",
			  query, query_len);
	if (dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
		wpa_printf(MSG_DEBUG, "P2P: Bonjour protocol not available");
		wpas_sd_add_proto_not_avail(resp, P2P_SERV_BONJOUR,
					    srv_trans_id);
		return;
	}

	if (query_len == 0) {
		wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
		return;
	}

	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (!match_bonjour_query(bsrv, query, query_len))
			continue;

		if (wpabuf_tailroom(resp) <
		    5 + query_len + wpabuf_len(bsrv->resp))
			return;

		matches++;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);
		wpa_hexdump_ascii(MSG_DEBUG, "P2P: Matching Bonjour service",
				  wpabuf_head(bsrv->resp),
				  wpabuf_len(bsrv->resp));

		/* Response Data */
		wpabuf_put_data(resp, query, query_len); /* Key */
		wpabuf_put_buf(resp, bsrv->resp); /* Value */

		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
	}

	if (matches == 0) {
		wpa_printf(MSG_DEBUG, "P2P: Requested Bonjour service not "
			   "available");
		if (wpabuf_tailroom(resp) < 5)
			return;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_REQUESTED_INFO_NOT_AVAILABLE);
		/* Response Data: empty */
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos -
			     2);
	}
}


static void wpas_sd_all_upnp(struct wpa_supplicant *wpa_s,
			     struct wpabuf *resp, u8 srv_trans_id)
{
	struct p2p_srv_upnp *usrv;
	u8 *len_pos;

	wpa_printf(MSG_DEBUG, "P2P: SD Request for all UPnP services");

	if (dl_list_empty(&wpa_s->global->p2p_srv_upnp)) {
		wpa_printf(MSG_DEBUG, "P2P: UPnP protocol not available");
		return;
	}

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (wpabuf_tailroom(resp) < 5 + 1 + os_strlen(usrv->service))
			return;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_UPNP);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);
		/* Response Data */
		wpabuf_put_u8(resp, usrv->version);
		wpa_printf(MSG_DEBUG, "P2P: Matching UPnP Service: %s",
			   usrv->service);
		wpabuf_put_str(resp, usrv->service);
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos -
			     2);
	}
}


static void wpas_sd_req_upnp(struct wpa_supplicant *wpa_s,
			     struct wpabuf *resp, u8 srv_trans_id,
			     const u8 *query, size_t query_len)
{
	struct p2p_srv_upnp *usrv;
	u8 *len_pos;
	u8 version;
	char *str;
	int count = 0;

	wpa_hexdump_ascii(MSG_DEBUG, "P2P: SD Request for UPnP",
			  query, query_len);

	if (dl_list_empty(&wpa_s->global->p2p_srv_upnp)) {
		wpa_printf(MSG_DEBUG, "P2P: UPnP protocol not available");
		wpas_sd_add_proto_not_avail(resp, P2P_SERV_UPNP,
					    srv_trans_id);
		return;
	}

	if (query_len == 0) {
		wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
		return;
	}

	if (wpabuf_tailroom(resp) < 5)
		return;

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, P2P_SERV_UPNP);
	wpabuf_put_u8(resp, srv_trans_id);

	version = query[0];
	str = os_malloc(query_len);
	if (str == NULL)
		return;
	os_memcpy(str, query + 1, query_len - 1);
	str[query_len - 1] = '\0';

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (version != usrv->version)
			continue;

		if (os_strcmp(str, "ssdp:all") != 0 &&
		    os_strstr(usrv->service, str) == NULL)
			continue;

		if (wpabuf_tailroom(resp) < 2)
			break;
		if (count == 0) {
			/* Status Code */
			wpabuf_put_u8(resp, P2P_SD_SUCCESS);
			/* Response Data */
			wpabuf_put_u8(resp, version);
		} else
			wpabuf_put_u8(resp, ',');

		count++;

		wpa_printf(MSG_DEBUG, "P2P: Matching UPnP Service: %s",
			   usrv->service);
		if (wpabuf_tailroom(resp) < os_strlen(usrv->service))
			break;
		wpabuf_put_str(resp, usrv->service);
	}
	os_free(str);

	if (count == 0) {
		wpa_printf(MSG_DEBUG, "P2P: Requested UPnP service not "
			   "available");
		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_REQUESTED_INFO_NOT_AVAILABLE);
		/* Response Data: empty */
	}

	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}


#ifdef CONFIG_WIFI_DISPLAY
static void wpas_sd_req_wfd(struct wpa_supplicant *wpa_s,
			    struct wpabuf *resp, u8 srv_trans_id,
			    const u8 *query, size_t query_len)
{
	const u8 *pos;
	u8 role;
	u8 *len_pos;

	wpa_hexdump(MSG_DEBUG, "P2P: SD Request for WFD", query, query_len);

	if (!wpa_s->global->wifi_display) {
		wpa_printf(MSG_DEBUG, "P2P: WFD protocol not available");
		wpas_sd_add_proto_not_avail(resp, P2P_SERV_WIFI_DISPLAY,
					    srv_trans_id);
		return;
	}

	if (query_len < 1) {
		wpa_printf(MSG_DEBUG, "P2P: Missing WFD Requested Device "
			   "Role");
		return;
	}

	if (wpabuf_tailroom(resp) < 5)
		return;

	pos = query;
	role = *pos++;
	wpa_printf(MSG_DEBUG, "P2P: WSD for device role 0x%x", role);

	/* TODO: role specific handling */

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, P2P_SERV_WIFI_DISPLAY);
	wpabuf_put_u8(resp, srv_trans_id);
	wpabuf_put_u8(resp, P2P_SD_SUCCESS); /* Status Code */

	while (pos < query + query_len) {
		if (*pos < MAX_WFD_SUBELEMS &&
		    wpa_s->global->wfd_subelem[*pos] &&
		    wpabuf_tailroom(resp) >=
		    wpabuf_len(wpa_s->global->wfd_subelem[*pos])) {
			wpa_printf(MSG_DEBUG, "P2P: Add WSD response "
				   "subelement %u", *pos);
			wpabuf_put_buf(resp, wpa_s->global->wfd_subelem[*pos]);
		}
		pos++;
	}

	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}
#endif /* CONFIG_WIFI_DISPLAY */


static void wpas_sd_request(void *ctx, int freq, const u8 *sa, u8 dialog_token,
			    u16 update_indic, const u8 *tlvs, size_t tlvs_len)
{
	struct wpa_supplicant *wpa_s = ctx;
	const u8 *pos = tlvs;
	const u8 *end = tlvs + tlvs_len;
	const u8 *tlv_end;
	u16 slen;
	struct wpabuf *resp;
	u8 srv_proto, srv_trans_id;
	size_t buf_len;
	char *buf;

	wpa_hexdump(MSG_MSGDUMP, "P2P: Service Discovery Request TLVs",
		    tlvs, tlvs_len);
	buf_len = 2 * tlvs_len + 1;
	buf = os_malloc(buf_len);
	if (buf) {
		wpa_snprintf_hex(buf, buf_len, tlvs, tlvs_len);
		wpa_msg_ctrl(wpa_s, MSG_INFO, P2P_EVENT_SERV_DISC_REQ "%d "
			     MACSTR " %u %u %s",
			     freq, MAC2STR(sa), dialog_token, update_indic,
			     buf);
		os_free(buf);
	}

	if (wpa_s->p2p_sd_over_ctrl_iface) {
		wpas_notify_p2p_sd_request(wpa_s, freq, sa, dialog_token,
					   update_indic, tlvs, tlvs_len);
		return; /* to be processed by an external program */
	}

	resp = wpabuf_alloc(10000);
	if (resp == NULL)
		return;

	while (pos + 1 < end) {
		wpa_printf(MSG_DEBUG, "P2P: Service Request TLV");
		slen = WPA_GET_LE16(pos);
		pos += 2;
		if (pos + slen > end || slen < 2) {
			wpa_printf(MSG_DEBUG, "P2P: Unexpected Query Data "
				   "length");
			wpabuf_free(resp);
			return;
		}
		tlv_end = pos + slen;

		srv_proto = *pos++;
		wpa_printf(MSG_DEBUG, "P2P: Service Protocol Type %u",
			   srv_proto);
		srv_trans_id = *pos++;
		wpa_printf(MSG_DEBUG, "P2P: Service Transaction ID %u",
			   srv_trans_id);

		wpa_hexdump(MSG_MSGDUMP, "P2P: Query Data",
			    pos, tlv_end - pos);


		if (wpa_s->force_long_sd) {
			wpa_printf(MSG_DEBUG, "P2P: SD test - force long "
				   "response");
			wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
			wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
			goto done;
		}

		switch (srv_proto) {
		case P2P_SERV_ALL_SERVICES:
			wpa_printf(MSG_DEBUG, "P2P: Service Discovery Request "
				   "for all services");
			if (dl_list_empty(&wpa_s->global->p2p_srv_upnp) &&
			    dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
				wpa_printf(MSG_DEBUG, "P2P: No service "
					   "discovery protocols available");
				wpas_sd_add_proto_not_avail(
					resp, P2P_SERV_ALL_SERVICES,
					srv_trans_id);
				break;
			}
			wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
			wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
			break;
		case P2P_SERV_BONJOUR:
			wpas_sd_req_bonjour(wpa_s, resp, srv_trans_id,
					    pos, tlv_end - pos);
			break;
		case P2P_SERV_UPNP:
			wpas_sd_req_upnp(wpa_s, resp, srv_trans_id,
					 pos, tlv_end - pos);
			break;
#ifdef CONFIG_WIFI_DISPLAY
		case P2P_SERV_WIFI_DISPLAY:
			wpas_sd_req_wfd(wpa_s, resp, srv_trans_id,
					pos, tlv_end - pos);
			break;
#endif /* CONFIG_WIFI_DISPLAY */
		default:
			wpa_printf(MSG_DEBUG, "P2P: Unavailable service "
				   "protocol %u", srv_proto);
			wpas_sd_add_proto_not_avail(resp, srv_proto,
						    srv_trans_id);
			break;
		}

		pos = tlv_end;
	}

done:
	wpas_notify_p2p_sd_request(wpa_s, freq, sa, dialog_token,
				   update_indic, tlvs, tlvs_len);

	wpas_p2p_sd_response(wpa_s, freq, sa, dialog_token, resp);

	wpabuf_free(resp);
}


static void wpas_sd_response(void *ctx, const u8 *sa, u16 update_indic,
			     const u8 *tlvs, size_t tlvs_len)
{
	struct wpa_supplicant *wpa_s = ctx;
	const u8 *pos = tlvs;
	const u8 *end = tlvs + tlvs_len;
	const u8 *tlv_end;
	u16 slen;
	size_t buf_len;
	char *buf;

	wpa_hexdump(MSG_MSGDUMP, "P2P: Service Discovery Response TLVs",
		    tlvs, tlvs_len);
	if (tlvs_len > 1500) {
		/* TODO: better way for handling this */
		wpa_msg_ctrl(wpa_s, MSG_INFO,
			     P2P_EVENT_SERV_DISC_RESP MACSTR
			     " %u <long response: %u bytes>",
			     MAC2STR(sa), update_indic,
			     (unsigned int) tlvs_len);
	} else {
		buf_len = 2 * tlvs_len + 1;
		buf = os_malloc(buf_len);
		if (buf) {
			wpa_snprintf_hex(buf, buf_len, tlvs, tlvs_len);
			wpa_msg_ctrl(wpa_s, MSG_INFO,
				     P2P_EVENT_SERV_DISC_RESP MACSTR " %u %s",
				     MAC2STR(sa), update_indic, buf);
			os_free(buf);
		}
	}

	while (pos < end) {
		u8 srv_proto, srv_trans_id, status;

		wpa_printf(MSG_DEBUG, "P2P: Service Response TLV");
		slen = WPA_GET_LE16(pos);
		pos += 2;
		if (pos + slen > end || slen < 3) {
			wpa_printf(MSG_DEBUG, "P2P: Unexpected Response Data "
				   "length");
			return;
		}
		tlv_end = pos + slen;

		srv_proto = *pos++;
		wpa_printf(MSG_DEBUG, "P2P: Service Protocol Type %u",
			   srv_proto);
		srv_trans_id = *pos++;
		wpa_printf(MSG_DEBUG, "P2P: Service Transaction ID %u",
			   srv_trans_id);
		status = *pos++;
		wpa_printf(MSG_DEBUG, "P2P: Status Code ID %u",
			   status);

		wpa_hexdump(MSG_MSGDUMP, "P2P: Response Data",
			    pos, tlv_end - pos);

		pos = tlv_end;
	}

	wpas_notify_p2p_sd_response(wpa_s, sa, update_indic, tlvs, tlvs_len);
}


u64 wpas_p2p_sd_request(struct wpa_supplicant *wpa_s, const u8 *dst,
			const struct wpabuf *tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;
	return (uintptr_t) p2p_sd_request(wpa_s->global->p2p, dst, tlvs);
}


u64 wpas_p2p_sd_request_upnp(struct wpa_supplicant *wpa_s, const u8 *dst,
			     u8 version, const char *query)
{
	struct wpabuf *tlvs;
	u64 ret;

	tlvs = wpabuf_alloc(2 + 1 + 1 + 1 + os_strlen(query));
	if (tlvs == NULL)
		return 0;
	wpabuf_put_le16(tlvs, 1 + 1 + 1 + os_strlen(query));
	wpabuf_put_u8(tlvs, P2P_SERV_UPNP); /* Service Protocol Type */
	wpabuf_put_u8(tlvs, 1); /* Service Transaction ID */
	wpabuf_put_u8(tlvs, version);
	wpabuf_put_str(tlvs, query);
	ret = wpas_p2p_sd_request(wpa_s, dst, tlvs);
	wpabuf_free(tlvs);
	return ret;
}


#ifdef CONFIG_WIFI_DISPLAY

static u64 wpas_p2p_sd_request_wfd(struct wpa_supplicant *wpa_s, const u8 *dst,
				   const struct wpabuf *tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;
	return (uintptr_t) p2p_sd_request_wfd(wpa_s->global->p2p, dst, tlvs);
}


#define MAX_WFD_SD_SUBELEMS 20

static void wfd_add_sd_req_role(struct wpabuf *tlvs, u8 id, u8 role,
				const char *subelems)
{
	u8 *len;
	const char *pos;
	int val;
	int count = 0;

	len = wpabuf_put(tlvs, 2);
	wpabuf_put_u8(tlvs, P2P_SERV_WIFI_DISPLAY); /* Service Protocol Type */
	wpabuf_put_u8(tlvs, id); /* Service Transaction ID */

	wpabuf_put_u8(tlvs, role);

	pos = subelems;
	while (*pos) {
		val = atoi(pos);
		if (val >= 0 && val < 256) {
			wpabuf_put_u8(tlvs, val);
			count++;
			if (count == MAX_WFD_SD_SUBELEMS)
				break;
		}
		pos = os_strchr(pos + 1, ',');
		if (pos == NULL)
			break;
		pos++;
	}

	WPA_PUT_LE16(len, (u8 *) wpabuf_put(tlvs, 0) - len - 2);
}


u64 wpas_p2p_sd_request_wifi_display(struct wpa_supplicant *wpa_s,
				     const u8 *dst, const char *role)
{
	struct wpabuf *tlvs;
	u64 ret;
	const char *subelems;
	u8 id = 1;

	subelems = os_strchr(role, ' ');
	if (subelems == NULL)
		return 0;
	subelems++;

	tlvs = wpabuf_alloc(4 * (2 + 1 + 1 + 1 + MAX_WFD_SD_SUBELEMS));
	if (tlvs == NULL)
		return 0;

	if (os_strstr(role, "[source]"))
		wfd_add_sd_req_role(tlvs, id++, 0x00, subelems);
	if (os_strstr(role, "[pri-sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x01, subelems);
	if (os_strstr(role, "[sec-sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x02, subelems);
	if (os_strstr(role, "[source+sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x03, subelems);

	ret = wpas_p2p_sd_request_wfd(wpa_s, dst, tlvs);
	wpabuf_free(tlvs);
	return ret;
}

#endif /* CONFIG_WIFI_DISPLAY */


int wpas_p2p_sd_cancel_request(struct wpa_supplicant *wpa_s, u64 req)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;
	return p2p_sd_cancel_request(wpa_s->global->p2p,
				     (void *) (uintptr_t) req);
}


void wpas_p2p_sd_response(struct wpa_supplicant *wpa_s, int freq,
			  const u8 *dst, u8 dialog_token,
			  const struct wpabuf *resp_tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;
	p2p_sd_response(wpa_s->global->p2p, freq, dst, dialog_token,
			resp_tlvs);
}


void wpas_p2p_sd_service_update(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->global->p2p)
		p2p_sd_service_update(wpa_s->global->p2p);
}


static void wpas_p2p_srv_bonjour_free(struct p2p_srv_bonjour *bsrv)
{
	dl_list_del(&bsrv->list);
	wpabuf_free(bsrv->query);
	wpabuf_free(bsrv->resp);
	os_free(bsrv);
}


static void wpas_p2p_srv_upnp_free(struct p2p_srv_upnp *usrv)
{
	dl_list_del(&usrv->list);
	os_free(usrv->service);
	os_free(usrv);
}


void wpas_p2p_service_flush(struct wpa_supplicant *wpa_s)
{
	struct p2p_srv_bonjour *bsrv, *bn;
	struct p2p_srv_upnp *usrv, *un;

	dl_list_for_each_safe(bsrv, bn, &wpa_s->global->p2p_srv_bonjour,
			      struct p2p_srv_bonjour, list)
		wpas_p2p_srv_bonjour_free(bsrv);

	dl_list_for_each_safe(usrv, un, &wpa_s->global->p2p_srv_upnp,
			      struct p2p_srv_upnp, list)
		wpas_p2p_srv_upnp_free(usrv);

	wpas_p2p_sd_service_update(wpa_s);
}


int wpas_p2p_service_add_bonjour(struct wpa_supplicant *wpa_s,
				 struct wpabuf *query, struct wpabuf *resp)
{
	struct p2p_srv_bonjour *bsrv;

	bsrv = os_zalloc(sizeof(*bsrv));
	if (bsrv == NULL)
		return -1;
	bsrv->query = query;
	bsrv->resp = resp;
	dl_list_add(&wpa_s->global->p2p_srv_bonjour, &bsrv->list);

	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_del_bonjour(struct wpa_supplicant *wpa_s,
				 const struct wpabuf *query)
{
	struct p2p_srv_bonjour *bsrv;

	bsrv = wpas_p2p_service_get_bonjour(wpa_s, query);
	if (bsrv == NULL)
		return -1;
	wpas_p2p_srv_bonjour_free(bsrv);
	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_add_upnp(struct wpa_supplicant *wpa_s, u8 version,
			      const char *service)
{
	struct p2p_srv_upnp *usrv;

	if (wpas_p2p_service_get_upnp(wpa_s, version, service))
		return 0; /* Already listed */
	usrv = os_zalloc(sizeof(*usrv));
	if (usrv == NULL)
		return -1;
	usrv->version = version;
	usrv->service = os_strdup(service);
	if (usrv->service == NULL) {
		os_free(usrv);
		return -1;
	}
	dl_list_add(&wpa_s->global->p2p_srv_upnp, &usrv->list);

	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_del_upnp(struct wpa_supplicant *wpa_s, u8 version,
			      const char *service)
{
	struct p2p_srv_upnp *usrv;

	usrv = wpas_p2p_service_get_upnp(wpa_s, version, service);
	if (usrv == NULL)
		return -1;
	wpas_p2p_srv_upnp_free(usrv);
	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


static void wpas_prov_disc_local_display(struct wpa_supplicant *wpa_s,
					 const u8 *peer, const char *params,
					 unsigned int generated_pin)
{
	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_PROV_DISC_SHOW_PIN MACSTR
		       " %08d%s", MAC2STR(peer), generated_pin, params);
}


static void wpas_prov_disc_local_keypad(struct wpa_supplicant *wpa_s,
					const u8 *peer, const char *params)
{
	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_PROV_DISC_ENTER_PIN MACSTR
		       "%s", MAC2STR(peer), params);
}


static void wpas_prov_disc_req(void *ctx, const u8 *peer, u16 config_methods,
			       const u8 *dev_addr, const u8 *pri_dev_type,
			       const char *dev_name, u16 supp_config_methods,
			       u8 dev_capab, u8 group_capab, const u8 *group_id,
			       size_t group_id_len)
{
	struct wpa_supplicant *wpa_s = ctx;
	char devtype[WPS_DEV_TYPE_BUFSIZE];
	char params[300];
	u8 empty_dev_type[8];
	unsigned int generated_pin = 0;
	struct wpa_supplicant *group = NULL;

	if (group_id) {
		for (group = wpa_s->global->ifaces; group; group = group->next)
		{
			struct wpa_ssid *s = group->current_ssid;
			if (s != NULL &&
			    s->mode == WPAS_MODE_P2P_GO &&
			    group_id_len - ETH_ALEN == s->ssid_len &&
			    os_memcmp(group_id + ETH_ALEN, s->ssid,
				      s->ssid_len) == 0)
				break;
		}
	}

	if (pri_dev_type == NULL) {
		os_memset(empty_dev_type, 0, sizeof(empty_dev_type));
		pri_dev_type = empty_dev_type;
	}
	os_snprintf(params, sizeof(params), " p2p_dev_addr=" MACSTR
		    " pri_dev_type=%s name='%s' config_methods=0x%x "
		    "dev_capab=0x%x group_capab=0x%x%s%s",
		    MAC2STR(dev_addr),
		    wps_dev_type_bin2str(pri_dev_type, devtype,
					 sizeof(devtype)),
		    dev_name, supp_config_methods, dev_capab, group_capab,
		    group ? " group=" : "",
		    group ? group->ifname : "");
	params[sizeof(params) - 1] = '\0';

	if (config_methods & WPS_CONFIG_DISPLAY) {
		generated_pin = wps_generate_pin();
		wpas_prov_disc_local_display(wpa_s, peer, params,
					     generated_pin);
	} else if (config_methods & WPS_CONFIG_KEYPAD)
		wpas_prov_disc_local_keypad(wpa_s, peer, params);
	else if (config_methods & WPS_CONFIG_PUSHBUTTON)
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_PROV_DISC_PBC_REQ
			       MACSTR "%s", MAC2STR(peer), params);

	wpas_notify_p2p_provision_discovery(wpa_s, peer, 1 /* request */,
					    P2P_PROV_DISC_SUCCESS,
					    config_methods, generated_pin);
}


static void wpas_prov_disc_resp(void *ctx, const u8 *peer, u16 config_methods)
{
	struct wpa_supplicant *wpa_s = ctx;
	unsigned int generated_pin = 0;
	char params[20];

	if (wpa_s->pending_pd_before_join &&
	    (os_memcmp(peer, wpa_s->pending_join_dev_addr, ETH_ALEN) == 0 ||
	     os_memcmp(peer, wpa_s->pending_join_iface_addr, ETH_ALEN) == 0)) {
		wpa_s->pending_pd_before_join = 0;
		wpa_printf(MSG_DEBUG, "P2P: Starting pending "
			   "join-existing-group operation");
		wpas_p2p_join_start(wpa_s, 0, NULL, 0);
		return;
	}

	if (wpa_s->pending_pd_use == AUTO_PD_JOIN ||
	    wpa_s->pending_pd_use == AUTO_PD_GO_NEG)
		os_snprintf(params, sizeof(params), " peer_go=%d",
			    wpa_s->pending_pd_use == AUTO_PD_JOIN);
	else
		params[0] = '\0';

	if (config_methods & WPS_CONFIG_DISPLAY)
		wpas_prov_disc_local_keypad(wpa_s, peer, params);
	else if (config_methods & WPS_CONFIG_KEYPAD) {
		generated_pin = wps_generate_pin();
		wpas_prov_disc_local_display(wpa_s, peer, params,
					     generated_pin);
	} else if (config_methods & WPS_CONFIG_PUSHBUTTON)
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_PROV_DISC_PBC_RESP
			       MACSTR "%s", MAC2STR(peer), params);

	wpas_notify_p2p_provision_discovery(wpa_s, peer, 0 /* response */,
					    P2P_PROV_DISC_SUCCESS,
					    config_methods, generated_pin);
}


static void wpas_prov_disc_fail(void *ctx, const u8 *peer,
				enum p2p_prov_disc_status status)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->p2p_fallback_to_go_neg) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: PD for p2p_connect-auto "
			"failed - fall back to GO Negotiation");
		wpas_p2p_fallback_to_go_neg(wpa_s, 0);
		return;
	}

	if (status == P2P_PROV_DISC_TIMEOUT_JOIN) {
		wpa_s->pending_pd_before_join = 0;
		wpa_printf(MSG_DEBUG, "P2P: Starting pending "
			   "join-existing-group operation (no ACK for PD "
			   "Req attempts)");
		wpas_p2p_join_start(wpa_s, 0, NULL, 0);
		return;
	}

	wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_PROV_DISC_FAILURE
		       " p2p_dev_addr=" MACSTR " status=%d",
		       MAC2STR(peer), status);

	wpas_notify_p2p_provision_discovery(wpa_s, peer, 0 /* response */,
					    status, 0, 0);
}


static int freq_included(const struct p2p_channels *channels, unsigned int freq)
{
	if (channels == NULL)
		return 1; /* Assume no restrictions */
	return p2p_channels_includes_freq(channels, freq);

}


/**
 * Pick the best frequency to use from all the currently used frequencies.
 */
static int wpas_p2p_pick_best_used_freq(struct wpa_supplicant *wpa_s,
					struct wpa_used_freq_data *freqs,
					unsigned int num)
{
	unsigned int i, c;

	/* find a candidate freq that is supported by P2P */
	for (c = 0; c < num; c++)
		if (p2p_supported_freq(wpa_s->global->p2p, freqs[c].freq))
			break;

	if (c == num)
		return 0;

	/* once we have a candidate, try to find a 'better' one */
	for (i = c + 1; i < num; i++) {
		if (!p2p_supported_freq(wpa_s->global->p2p, freqs[i].freq))
			continue;

		/*
		 * 1. Infrastructure station interfaces have higher preference.
		 * 2. P2P Clients have higher preference.
		 * 3. All others.
		 */
		if (freqs[i].flags & WPA_FREQ_USED_BY_INFRA_STATION) {
			c = i;
			break;
		}

		if ((freqs[i].flags & WPA_FREQ_USED_BY_P2P_CLIENT))
			c = i;
	}
	return freqs[c].freq;
}


static u8 wpas_invitation_process(void *ctx, const u8 *sa, const u8 *bssid,
				  const u8 *go_dev_addr, const u8 *ssid,
				  size_t ssid_len, int *go, u8 *group_bssid,
				  int *force_freq, int persistent_group,
				  const struct p2p_channels *channels,
				  int dev_pw_id)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *s;
	struct wpa_used_freq_data *freqs;
	struct wpa_supplicant *grp;
	int best_freq;

	if (!persistent_group) {
		wpa_printf(MSG_DEBUG, "P2P: Invitation from " MACSTR
			   " to join an active group (SSID: %s)",
			   MAC2STR(sa), wpa_ssid_txt(ssid, ssid_len));
		if (!is_zero_ether_addr(wpa_s->p2p_auth_invite) &&
		    (os_memcmp(go_dev_addr, wpa_s->p2p_auth_invite, ETH_ALEN)
		     == 0 ||
		     os_memcmp(sa, wpa_s->p2p_auth_invite, ETH_ALEN) == 0)) {
			wpa_printf(MSG_DEBUG, "P2P: Accept previously "
				   "authorized invitation");
			goto accept_inv;
		}

#ifdef CONFIG_WPS_NFC
		if (dev_pw_id >= 0 && wpa_s->parent->p2p_nfc_tag_enabled &&
		    dev_pw_id == wpa_s->parent->p2p_oob_dev_pw_id) {
			wpa_printf(MSG_DEBUG, "P2P: Accept invitation based on local enabled NFC Tag");
			wpa_s->parent->p2p_wps_method = WPS_NFC;
			wpa_s->parent->pending_join_wps_method = WPS_NFC;
			os_memcpy(wpa_s->parent->pending_join_dev_addr,
				  go_dev_addr, ETH_ALEN);
			os_memcpy(wpa_s->parent->pending_join_iface_addr,
				  bssid, ETH_ALEN);
			goto accept_inv;
		}
#endif /* CONFIG_WPS_NFC */

		/*
		 * Do not accept the invitation automatically; notify user and
		 * request approval.
		 */
		return P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE;
	}

	grp = wpas_get_p2p_group(wpa_s, ssid, ssid_len, go);
	if (grp) {
		wpa_printf(MSG_DEBUG, "P2P: Accept invitation to already "
			   "running persistent group");
		if (*go)
			os_memcpy(group_bssid, grp->own_addr, ETH_ALEN);
		goto accept_inv;
	}

	if (!is_zero_ether_addr(wpa_s->p2p_auth_invite) &&
	    os_memcmp(sa, wpa_s->p2p_auth_invite, ETH_ALEN) == 0) {
		wpa_printf(MSG_DEBUG, "P2P: Accept previously initiated "
			   "invitation to re-invoke a persistent group");
		os_memset(wpa_s->p2p_auth_invite, 0, ETH_ALEN);
	} else if (!wpa_s->conf->persistent_reconnect)
		return P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    os_memcmp(s->bssid, go_dev_addr, ETH_ALEN) == 0 &&
		    s->ssid_len == ssid_len &&
		    os_memcmp(ssid, s->ssid, ssid_len) == 0)
			break;
	}

	if (!s) {
		wpa_printf(MSG_DEBUG, "P2P: Invitation from " MACSTR
			   " requested reinvocation of an unknown group",
			   MAC2STR(sa));
		return P2P_SC_FAIL_UNKNOWN_GROUP;
	}

	if (s->mode == WPAS_MODE_P2P_GO && !wpas_p2p_create_iface(wpa_s)) {
		*go = 1;
		if (wpa_s->wpa_state >= WPA_AUTHENTICATING) {
			wpa_printf(MSG_DEBUG, "P2P: The only available "
				   "interface is already in use - reject "
				   "invitation");
			return P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE;
		}
		os_memcpy(group_bssid, wpa_s->own_addr, ETH_ALEN);
	} else if (s->mode == WPAS_MODE_P2P_GO) {
		*go = 1;
		if (wpas_p2p_add_group_interface(wpa_s, WPA_IF_P2P_GO) < 0)
		{
			wpa_printf(MSG_ERROR, "P2P: Failed to allocate a new "
				   "interface address for the group");
			return P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE;
		}
		os_memcpy(group_bssid, wpa_s->pending_interface_addr,
			  ETH_ALEN);
	}

accept_inv:
	wpas_p2p_set_own_freq_preference(wpa_s, 0);

	best_freq = 0;
	freqs = os_calloc(wpa_s->num_multichan_concurrent,
			  sizeof(struct wpa_used_freq_data));
	if (freqs) {
		int num_channels = wpa_s->num_multichan_concurrent;
		int num = wpas_p2p_valid_oper_freqs(wpa_s, freqs, num_channels);
		best_freq = wpas_p2p_pick_best_used_freq(wpa_s, freqs, num);
		os_free(freqs);
	}

	/* Get one of the frequencies currently in use */
	if (best_freq > 0) {
		wpa_printf(MSG_DEBUG, "P2P: Trying to prefer a channel already used by one of the interfaces");
		wpas_p2p_set_own_freq_preference(wpa_s, best_freq);

		if (wpa_s->num_multichan_concurrent < 2 ||
		    wpas_p2p_num_unused_channels(wpa_s) < 1) {
			wpa_printf(MSG_DEBUG, "P2P: No extra channels available - trying to force channel to match a channel already used by one of the interfaces");
			*force_freq = best_freq;
		}
	}

	if (*force_freq > 0 && wpa_s->num_multichan_concurrent > 1 &&
	    wpas_p2p_num_unused_channels(wpa_s) > 0) {
		if (*go == 0) {
			/* We are the client */
			wpa_printf(MSG_DEBUG, "P2P: Peer was found to be "
				   "running a GO but we are capable of MCC, "
				   "figure out the best channel to use");
			*force_freq = 0;
		} else if (!freq_included(channels, *force_freq)) {
			/* We are the GO, and *force_freq is not in the
			 * intersection */
			wpa_printf(MSG_DEBUG, "P2P: Forced GO freq %d MHz not "
				   "in intersection but we are capable of MCC, "
				   "figure out the best channel to use",
				   *force_freq);
			*force_freq = 0;
		}
	}

	return P2P_SC_SUCCESS;
}


static void wpas_invitation_received(void *ctx, const u8 *sa, const u8 *bssid,
				     const u8 *ssid, size_t ssid_len,
				     const u8 *go_dev_addr, u8 status,
				     int op_freq)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *s;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    s->ssid_len == ssid_len &&
		    os_memcmp(ssid, s->ssid, ssid_len) == 0)
			break;
	}

	if (status == P2P_SC_SUCCESS) {
		wpa_printf(MSG_DEBUG, "P2P: Invitation from peer " MACSTR
			   " was accepted; op_freq=%d MHz, SSID=%s",
			   MAC2STR(sa), op_freq, wpa_ssid_txt(ssid, ssid_len));
		if (s) {
			int go = s->mode == WPAS_MODE_P2P_GO;
			wpas_p2p_group_add_persistent(
				wpa_s, s, go, 0, op_freq, 0, 0, NULL,
				go ? P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE : 0);
		} else if (bssid) {
			wpa_s->user_initiated_pd = 0;
			wpas_p2p_join(wpa_s, bssid, go_dev_addr,
				      wpa_s->p2p_wps_method, 0, op_freq,
				      ssid, ssid_len);
		}
		return;
	}

	if (status != P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE) {
		wpa_printf(MSG_DEBUG, "P2P: Invitation from peer " MACSTR
			   " was rejected (status %u)", MAC2STR(sa), status);
		return;
	}

	if (!s) {
		if (bssid) {
			wpa_msg_global(wpa_s, MSG_INFO,
				       P2P_EVENT_INVITATION_RECEIVED
				       "sa=" MACSTR " go_dev_addr=" MACSTR
				       " bssid=" MACSTR " unknown-network",
				       MAC2STR(sa), MAC2STR(go_dev_addr),
				       MAC2STR(bssid));
		} else {
			wpa_msg_global(wpa_s, MSG_INFO,
				       P2P_EVENT_INVITATION_RECEIVED
				       "sa=" MACSTR " go_dev_addr=" MACSTR
				       " unknown-network",
				       MAC2STR(sa), MAC2STR(go_dev_addr));
		}
		return;
	}

	if (s->mode == WPAS_MODE_P2P_GO && op_freq) {
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_INVITATION_RECEIVED
			       "sa=" MACSTR " persistent=%d freq=%d",
			       MAC2STR(sa), s->id, op_freq);
	} else {
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_INVITATION_RECEIVED
			       "sa=" MACSTR " persistent=%d",
			       MAC2STR(sa), s->id);
	}
}


static void wpas_remove_persistent_peer(struct wpa_supplicant *wpa_s,
					struct wpa_ssid *ssid,
					const u8 *peer, int inv)
{
	size_t i;

	if (ssid == NULL)
		return;

	for (i = 0; ssid->p2p_client_list && i < ssid->num_p2p_clients; i++) {
		if (os_memcmp(ssid->p2p_client_list + i * ETH_ALEN, peer,
			      ETH_ALEN) == 0)
			break;
	}
	if (i >= ssid->num_p2p_clients || !ssid->p2p_client_list) {
		if (ssid->mode != WPAS_MODE_P2P_GO &&
		    os_memcmp(ssid->bssid, peer, ETH_ALEN) == 0) {
			wpa_printf(MSG_DEBUG, "P2P: Remove persistent group %d "
				   "due to invitation result", ssid->id);
			wpas_notify_network_removed(wpa_s, ssid);
			wpa_config_remove_network(wpa_s->conf, ssid->id);
			return;
		}
		return; /* Peer not found in client list */
	}

	wpa_printf(MSG_DEBUG, "P2P: Remove peer " MACSTR " from persistent "
		   "group %d client list%s",
		   MAC2STR(peer), ssid->id,
		   inv ? " due to invitation result" : "");
	os_memmove(ssid->p2p_client_list + i * ETH_ALEN,
		   ssid->p2p_client_list + (i + 1) * ETH_ALEN,
		   (ssid->num_p2p_clients - i - 1) * ETH_ALEN);
	ssid->num_p2p_clients--;
	if (wpa_s->parent->conf->update_config &&
	    wpa_config_write(wpa_s->parent->confname, wpa_s->parent->conf))
		wpa_printf(MSG_DEBUG, "P2P: Failed to update configuration");
}


static void wpas_remove_persistent_client(struct wpa_supplicant *wpa_s,
					  const u8 *peer)
{
	struct wpa_ssid *ssid;

	wpa_s = wpa_s->global->p2p_invite_group;
	if (wpa_s == NULL)
		return; /* No known invitation group */
	ssid = wpa_s->current_ssid;
	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GO ||
	    !ssid->p2p_persistent_group)
		return; /* Not operating as a GO in persistent group */
	ssid = wpas_p2p_get_persistent(wpa_s->parent, peer,
				       ssid->ssid, ssid->ssid_len);
	wpas_remove_persistent_peer(wpa_s, ssid, peer, 1);
}


static void wpas_invitation_result(void *ctx, int status, const u8 *bssid,
				   const struct p2p_channels *channels,
				   const u8 *peer, int neg_freq,
				   int peer_oper_freq)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *ssid;
	int freq;

	if (bssid) {
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_INVITATION_RESULT
			       "status=%d " MACSTR,
			       status, MAC2STR(bssid));
	} else {
		wpa_msg_global(wpa_s, MSG_INFO, P2P_EVENT_INVITATION_RESULT
			       "status=%d ", status);
	}
	wpas_notify_p2p_invitation_result(wpa_s, status, bssid);

	wpa_printf(MSG_DEBUG, "P2P: Invitation result - status=%d peer=" MACSTR,
		   status, MAC2STR(peer));
	if (wpa_s->pending_invite_ssid_id == -1) {
		if (status == P2P_SC_FAIL_UNKNOWN_GROUP)
			wpas_remove_persistent_client(wpa_s, peer);
		return; /* Invitation to active group */
	}

	if (status == P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE) {
		wpa_printf(MSG_DEBUG, "P2P: Waiting for peer to start another "
			   "invitation exchange to indicate readiness for "
			   "re-invocation");
	}

	if (status != P2P_SC_SUCCESS) {
		if (status == P2P_SC_FAIL_UNKNOWN_GROUP) {
			ssid = wpa_config_get_network(
				wpa_s->conf, wpa_s->pending_invite_ssid_id);
			wpas_remove_persistent_peer(wpa_s, ssid, peer, 1);
		}
		wpas_p2p_remove_pending_group_interface(wpa_s);
		return;
	}

	ssid = wpa_config_get_network(wpa_s->conf,
				      wpa_s->pending_invite_ssid_id);
	if (ssid == NULL) {
		wpa_printf(MSG_ERROR, "P2P: Could not find persistent group "
			   "data matching with invitation");
		return;
	}

	/*
	 * The peer could have missed our ctrl::ack frame for Invitation
	 * Response and continue retransmitting the frame. To reduce the
	 * likelihood of the peer not getting successful TX status for the
	 * Invitation Response frame, wait a short time here before starting
	 * the persistent group so that we will remain on the current channel to
	 * acknowledge any possible retransmission from the peer.
	 */
	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: 50 ms wait on current channel before "
		"starting persistent group");
	os_sleep(0, 50000);

	if (neg_freq > 0 && ssid->mode == WPAS_MODE_P2P_GO &&
	    freq_included(channels, neg_freq))
		freq = neg_freq;
	else if (peer_oper_freq > 0 && ssid->mode != WPAS_MODE_P2P_GO &&
		 freq_included(channels, peer_oper_freq))
		freq = peer_oper_freq;
	else
		freq = 0;

	wpa_printf(MSG_DEBUG, "P2P: Persistent group invitation success - op_freq=%d MHz SSID=%s",
		   freq, wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
	wpas_p2p_group_add_persistent(wpa_s, ssid,
				      ssid->mode == WPAS_MODE_P2P_GO,
				      wpa_s->p2p_persistent_go_freq,
				      freq,
				      wpa_s->p2p_go_ht40, wpa_s->p2p_go_vht,
				      channels,
				      ssid->mode == WPAS_MODE_P2P_GO ?
				      P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE :
				      0);
}


static int wpas_p2p_disallowed_freq(struct wpa_global *global,
				    unsigned int freq)
{
	if (freq_range_list_includes(&global->p2p_go_avoid_freq, freq))
		return 1;
	return freq_range_list_includes(&global->p2p_disallow_freq, freq);
}


static void wpas_p2p_add_chan(struct p2p_reg_class *reg, u8 chan)
{
	reg->channel[reg->channels] = chan;
	reg->channels++;
}


static int wpas_p2p_default_channels(struct wpa_supplicant *wpa_s,
				     struct p2p_channels *chan,
				     struct p2p_channels *cli_chan)
{
	int i, cla = 0;

	os_memset(cli_chan, 0, sizeof(*cli_chan));

	wpa_printf(MSG_DEBUG, "P2P: Enable operating classes for 2.4 GHz "
		   "band");

	/* Operating class 81 - 2.4 GHz band channels 1..13 */
	chan->reg_class[cla].reg_class = 81;
	chan->reg_class[cla].channels = 0;
	for (i = 0; i < 11; i++) {
		if (!wpas_p2p_disallowed_freq(wpa_s->global, 2412 + i * 5))
			wpas_p2p_add_chan(&chan->reg_class[cla], i + 1);
	}
	if (chan->reg_class[cla].channels)
		cla++;

	wpa_printf(MSG_DEBUG, "P2P: Enable operating classes for lower 5 GHz "
		   "band");

	/* Operating class 115 - 5 GHz, channels 36-48 */
	chan->reg_class[cla].reg_class = 115;
	chan->reg_class[cla].channels = 0;
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 36 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 36);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 40 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 40);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 44 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 44);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 48 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 48);
	if (chan->reg_class[cla].channels)
		cla++;

	wpa_printf(MSG_DEBUG, "P2P: Enable operating classes for higher 5 GHz "
		   "band");

	/* Operating class 124 - 5 GHz, channels 149,153,157,161 */
	chan->reg_class[cla].reg_class = 124;
	chan->reg_class[cla].channels = 0;
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 149 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 149);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 153 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 153);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 156 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 157);
	if (!wpas_p2p_disallowed_freq(wpa_s->global, 5000 + 161 * 5))
		wpas_p2p_add_chan(&chan->reg_class[cla], 161);
	if (chan->reg_class[cla].channels)
		cla++;

	chan->reg_classes = cla;
	return 0;
}


static struct hostapd_hw_modes * get_mode(struct hostapd_hw_modes *modes,
					  u16 num_modes,
					  enum hostapd_hw_mode mode)
{
	u16 i;

	for (i = 0; i < num_modes; i++) {
		if (modes[i].mode == mode)
			return &modes[i];
	}

	return NULL;
}


enum chan_allowed {
	NOT_ALLOWED, PASSIVE_ONLY, ALLOWED
};

static int has_channel(struct wpa_global *global,
		       struct hostapd_hw_modes *mode, u8 chan, int *flags)
{
	int i;
	unsigned int freq;

	freq = (mode->mode == HOSTAPD_MODE_IEEE80211A ? 5000 : 2407) +
		chan * 5;
	if (wpas_p2p_disallowed_freq(global, freq))
		return NOT_ALLOWED;

	for (i = 0; i < mode->num_channels; i++) {
		if (mode->channels[i].chan == chan) {
			if (flags)
				*flags = mode->channels[i].flag;
			if (mode->channels[i].flag &
			    (HOSTAPD_CHAN_DISABLED |
			     HOSTAPD_CHAN_RADAR))
				return NOT_ALLOWED;
			if (mode->channels[i].flag &
			    (HOSTAPD_CHAN_PASSIVE_SCAN |
			     HOSTAPD_CHAN_NO_IBSS))
				return PASSIVE_ONLY;
			return ALLOWED;
		}
	}

	return NOT_ALLOWED;
}


struct p2p_oper_class_map {
	enum hostapd_hw_mode mode;
	u8 op_class;
	u8 min_chan;
	u8 max_chan;
	u8 inc;
	enum { BW20, BW40PLUS, BW40MINUS, BW80, BW2160 } bw;
};

static struct p2p_oper_class_map op_class[] = {
	{ HOSTAPD_MODE_IEEE80211G, 81, 1, 13, 1, BW20 },
#if 0 /* Do not enable HT40 on 2 GHz for now */
	{ HOSTAPD_MODE_IEEE80211G, 83, 1, 9, 1, BW40PLUS },
	{ HOSTAPD_MODE_IEEE80211G, 84, 5, 13, 1, BW40MINUS },
#endif
	{ HOSTAPD_MODE_IEEE80211A, 115, 36, 48, 4, BW20 },
	{ HOSTAPD_MODE_IEEE80211A, 124, 149, 161, 4, BW20 },
	{ HOSTAPD_MODE_IEEE80211A, 116, 36, 44, 8, BW40PLUS },
	{ HOSTAPD_MODE_IEEE80211A, 117, 40, 48, 8, BW40MINUS },
	{ HOSTAPD_MODE_IEEE80211A, 126, 149, 157, 8, BW40PLUS },
	{ HOSTAPD_MODE_IEEE80211A, 127, 153, 161, 8, BW40MINUS },

	/*
	 * IEEE P802.11ac/D7.0 Table E-4 actually talks about channel center
	 * frequency index 42, 58, 106, 122, 138, 155 with channel spacing of
	 * 80 MHz, but currently use the following definition for simplicity
	 * (these center frequencies are not actual channels, which makes
	 * has_channel() fail). wpas_p2p_verify_80mhz() should take care of
	 * removing invalid channels.
	 */
	{ HOSTAPD_MODE_IEEE80211A, 128, 36, 161, 4, BW80 },
	{ HOSTAPD_MODE_IEEE80211AD, 180, 1, 4, 1, BW2160 },
	{ -1, 0, 0, 0, 0, BW20 }
};


static int wpas_p2p_get_center_80mhz(struct wpa_supplicant *wpa_s,
				     struct hostapd_hw_modes *mode,
				     u8 channel)
{
	u8 center_channels[] = { 42, 58, 106, 122, 138, 155 };
	unsigned int i;

	if (mode->mode != HOSTAPD_MODE_IEEE80211A)
		return 0;

	for (i = 0; i < ARRAY_SIZE(center_channels); i++)
		/*
		 * In 80 MHz, the bandwidth "spans" 12 channels (e.g., 36-48),
		 * so the center channel is 6 channels away from the start/end.
		 */
		if (channel >= center_channels[i] - 6 &&
		    channel <= center_channels[i] + 6)
			return center_channels[i];

	return 0;
}


static enum chan_allowed wpas_p2p_verify_80mhz(struct wpa_supplicant *wpa_s,
					       struct hostapd_hw_modes *mode,
					       u8 channel, u8 bw)
{
	u8 center_chan;
	int i, flags;
	enum chan_allowed res, ret = ALLOWED;

	center_chan = wpas_p2p_get_center_80mhz(wpa_s, mode, channel);
	if (!center_chan)
		return NOT_ALLOWED;
	if (center_chan >= 58 && center_chan <= 138)
		return NOT_ALLOWED; /* Do not allow DFS channels for P2P */

	/* check all the channels are available */
	for (i = 0; i < 4; i++) {
		int adj_chan = center_chan - 6 + i * 4;

		res = has_channel(wpa_s->global, mode, adj_chan, &flags);
		if (res == NOT_ALLOWED)
			return NOT_ALLOWED;
		if (res == PASSIVE_ONLY)
			ret = PASSIVE_ONLY;

		if (i == 0 && !(flags & HOSTAPD_CHAN_VHT_10_70))
			return NOT_ALLOWED;
		if (i == 1 && !(flags & HOSTAPD_CHAN_VHT_30_50))
			return NOT_ALLOWED;
		if (i == 2 && !(flags & HOSTAPD_CHAN_VHT_50_30))
			return NOT_ALLOWED;
		if (i == 3 && !(flags & HOSTAPD_CHAN_VHT_70_10))
			return NOT_ALLOWED;
	}

	return ret;
}


static enum chan_allowed wpas_p2p_verify_channel(struct wpa_supplicant *wpa_s,
						 struct hostapd_hw_modes *mode,
						 u8 channel, u8 bw)
{
	int flag = 0;
	enum chan_allowed res, res2;

	res2 = res = has_channel(wpa_s->global, mode, channel, &flag);
	if (bw == BW40MINUS) {
		if (!(flag & HOSTAPD_CHAN_HT40MINUS))
			return NOT_ALLOWED;
		res2 = has_channel(wpa_s->global, mode, channel - 4, NULL);
	} else if (bw == BW40PLUS) {
		if (!(flag & HOSTAPD_CHAN_HT40PLUS))
			return NOT_ALLOWED;
		res2 = has_channel(wpa_s->global, mode, channel + 4, NULL);
	} else if (bw == BW80) {
		res2 = wpas_p2p_verify_80mhz(wpa_s, mode, channel, bw);
	}

	if (res == NOT_ALLOWED || res2 == NOT_ALLOWED)
		return NOT_ALLOWED;
	if (res == PASSIVE_ONLY || res2 == PASSIVE_ONLY)
		return PASSIVE_ONLY;
	return res;
}


static int wpas_p2p_setup_channels(struct wpa_supplicant *wpa_s,
				   struct p2p_channels *chan,
				   struct p2p_channels *cli_chan)
{
	struct hostapd_hw_modes *mode;
	int cla, op, cli_cla;

	if (wpa_s->hw.modes == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Driver did not support fetching "
			   "of all supported channels; assume dualband "
			   "support");
		return wpas_p2p_default_channels(wpa_s, chan, cli_chan);
	}

	cla = cli_cla = 0;

	for (op = 0; op_class[op].op_class; op++) {
		struct p2p_oper_class_map *o = &op_class[op];
		u8 ch;
		struct p2p_reg_class *reg = NULL, *cli_reg = NULL;

		mode = get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes, o->mode);
		if (mode == NULL)
			continue;
		for (ch = o->min_chan; ch <= o->max_chan; ch += o->inc) {
			enum chan_allowed res;
			res = wpas_p2p_verify_channel(wpa_s, mode, ch, o->bw);
			if (res == ALLOWED) {
				if (reg == NULL) {
					wpa_printf(MSG_DEBUG, "P2P: Add operating class %u",
						   o->op_class);
					reg = &chan->reg_class[cla];
					cla++;
					reg->reg_class = o->op_class;
				}
				reg->channel[reg->channels] = ch;
				reg->channels++;
			} else if (res == PASSIVE_ONLY &&
				   wpa_s->conf->p2p_add_cli_chan) {
				if (cli_reg == NULL) {
					wpa_printf(MSG_DEBUG, "P2P: Add operating class %u (client only)",
						   o->op_class);
					cli_reg = &cli_chan->reg_class[cli_cla];
					cli_cla++;
					cli_reg->reg_class = o->op_class;
				}
				cli_reg->channel[cli_reg->channels] = ch;
				cli_reg->channels++;
			}
		}
		if (reg) {
			wpa_hexdump(MSG_DEBUG, "P2P: Channels",
				    reg->channel, reg->channels);
		}
		if (cli_reg) {
			wpa_hexdump(MSG_DEBUG, "P2P: Channels (client only)",
				    cli_reg->channel, cli_reg->channels);
		}
	}

	chan->reg_classes = cla;
	cli_chan->reg_classes = cli_cla;

	return 0;
}


int wpas_p2p_get_ht40_mode(struct wpa_supplicant *wpa_s,
			   struct hostapd_hw_modes *mode, u8 channel)
{
	int op;
	enum chan_allowed ret;

	for (op = 0; op_class[op].op_class; op++) {
		struct p2p_oper_class_map *o = &op_class[op];
		u8 ch;

		for (ch = o->min_chan; ch <= o->max_chan; ch += o->inc) {
			if (o->mode != HOSTAPD_MODE_IEEE80211A ||
			    o->bw == BW20 || ch != channel)
				continue;
			ret = wpas_p2p_verify_channel(wpa_s, mode, ch, o->bw);
			if (ret == ALLOWED)
				return (o->bw == BW40MINUS) ? -1 : 1;
		}
	}
	return 0;
}


int wpas_p2p_get_vht80_center(struct wpa_supplicant *wpa_s,
			      struct hostapd_hw_modes *mode, u8 channel)
{
	if (!wpas_p2p_verify_channel(wpa_s, mode, channel, BW80))
		return 0;

	return wpas_p2p_get_center_80mhz(wpa_s, mode, channel);
}


static int wpas_get_noa(void *ctx, const u8 *interface_addr, u8 *buf,
			size_t buf_len)
{
	struct wpa_supplicant *wpa_s = ctx;

	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_memcmp(wpa_s->own_addr, interface_addr, ETH_ALEN) == 0)
			break;
	}
	if (wpa_s == NULL)
		return -1;

	return wpa_drv_get_noa(wpa_s, buf, buf_len);
}


struct wpa_supplicant * wpas_get_p2p_go_iface(struct wpa_supplicant *wpa_s,
					      const u8 *ssid, size_t ssid_len)
{
	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		struct wpa_ssid *s = wpa_s->current_ssid;
		if (s == NULL)
			continue;
		if (s->mode != WPAS_MODE_P2P_GO &&
		    s->mode != WPAS_MODE_AP &&
		    s->mode != WPAS_MODE_P2P_GROUP_FORMATION)
			continue;
		if (s->ssid_len != ssid_len ||
		    os_memcmp(ssid, s->ssid, ssid_len) != 0)
			continue;
		return wpa_s;
	}

	return NULL;

}


struct wpa_supplicant * wpas_get_p2p_client_iface(struct wpa_supplicant *wpa_s,
						  const u8 *peer_dev_addr)
{
	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		if (ssid == NULL)
			continue;
		if (ssid->mode != WPAS_MODE_INFRA)
			continue;
		if (wpa_s->wpa_state != WPA_COMPLETED &&
		    wpa_s->wpa_state != WPA_GROUP_HANDSHAKE)
			continue;
		if (os_memcmp(wpa_s->go_dev_addr, peer_dev_addr, ETH_ALEN) == 0)
			return wpa_s;
	}

	return NULL;
}


static int wpas_go_connected(void *ctx, const u8 *dev_addr)
{
	struct wpa_supplicant *wpa_s = ctx;

	return wpas_get_p2p_client_iface(wpa_s, dev_addr) != NULL;
}


static int wpas_is_concurrent_session_active(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_supplicant *ifs;

	for (ifs = wpa_s->global->ifaces; ifs; ifs = ifs->next) {
		if (ifs == wpa_s)
			continue;
		if (ifs->wpa_state > WPA_ASSOCIATED)
			return 1;
	}
	return 0;
}


static void wpas_p2p_debug_print(void *ctx, int level, const char *msg)
{
	struct wpa_supplicant *wpa_s = ctx;
	wpa_msg_global(wpa_s, level, "P2P: %s", msg);
}


int wpas_p2p_add_p2pdev_interface(struct wpa_supplicant *wpa_s,
				  const char *conf_p2p_dev)
{
	struct wpa_interface iface;
	struct wpa_supplicant *p2pdev_wpa_s;
	char ifname[100];
	char force_name[100];
	int ret;

	os_snprintf(ifname, sizeof(ifname), P2P_MGMT_DEVICE_PREFIX "%s",
		    wpa_s->ifname);
	force_name[0] = '\0';
	wpa_s->pending_interface_type = WPA_IF_P2P_DEVICE;
	ret = wpa_drv_if_add(wpa_s, WPA_IF_P2P_DEVICE, ifname, NULL, NULL,
			     force_name, wpa_s->pending_interface_addr, NULL);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to create P2P Device interface");
		return ret;
	}
	os_strlcpy(wpa_s->pending_interface_name, ifname,
		   sizeof(wpa_s->pending_interface_name));

	os_memset(&iface, 0, sizeof(iface));
	iface.p2p_mgmt = 1;
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;
	iface.driver_param = wpa_s->conf->driver_param;

	/*
	 * If a P2P Device configuration file was given, use it as the interface
	 * configuration file (instead of using parent's configuration file.
	 */
	if (conf_p2p_dev) {
		iface.confname = conf_p2p_dev;
		iface.ctrl_interface = NULL;
	} else {
		iface.confname = wpa_s->confname;
		iface.ctrl_interface = wpa_s->conf->ctrl_interface;
	}
	iface.conf_p2p_dev = NULL;

	p2pdev_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface);
	if (!p2pdev_wpa_s) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to add P2P Device interface");
		return -1;
	}
	p2pdev_wpa_s->parent = wpa_s;

	wpa_s->pending_interface_name[0] = '\0';
	return 0;
}


static void wpas_presence_resp(void *ctx, const u8 *src, u8 status,
			       const u8 *noa, size_t noa_len)
{
	struct wpa_supplicant *wpa_s, *intf = ctx;
	char hex[100];

	for (wpa_s = intf->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s->waiting_presence_resp)
			break;
	}
	if (!wpa_s) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: No group interface was waiting for presence response");
		return;
	}
	wpa_s->waiting_presence_resp = 0;

	wpa_snprintf_hex(hex, sizeof(hex), noa, noa_len);
	wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_PRESENCE_RESPONSE "src=" MACSTR
		" status=%u noa=%s", MAC2STR(src), status, hex);
}


static int _wpas_p2p_in_progress(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	return wpas_p2p_in_progress(wpa_s);
}


/**
 * wpas_p2p_init - Initialize P2P module for %wpa_supplicant
 * @global: Pointer to global data from wpa_supplicant_init()
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * Returns: 0 on success, -1 on failure
 */
int wpas_p2p_init(struct wpa_global *global, struct wpa_supplicant *wpa_s)
{
	struct p2p_config p2p;
	int i;

	if (wpa_s->conf->p2p_disabled)
		return 0;

	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CAPABLE))
		return 0;

	if (global->p2p)
		return 0;

	os_memset(&p2p, 0, sizeof(p2p));
	p2p.cb_ctx = wpa_s;
	p2p.debug_print = wpas_p2p_debug_print;
	p2p.p2p_scan = wpas_p2p_scan;
	p2p.send_action = wpas_send_action;
	p2p.send_action_done = wpas_send_action_done;
	p2p.go_neg_completed = wpas_go_neg_completed;
	p2p.go_neg_req_rx = wpas_go_neg_req_rx;
	p2p.dev_found = wpas_dev_found;
	p2p.dev_lost = wpas_dev_lost;
	p2p.find_stopped = wpas_find_stopped;
	p2p.start_listen = wpas_start_listen;
	p2p.stop_listen = wpas_stop_listen;
	p2p.send_probe_resp = wpas_send_probe_resp;
	p2p.sd_request = wpas_sd_request;
	p2p.sd_response = wpas_sd_response;
	p2p.prov_disc_req = wpas_prov_disc_req;
	p2p.prov_disc_resp = wpas_prov_disc_resp;
	p2p.prov_disc_fail = wpas_prov_disc_fail;
	p2p.invitation_process = wpas_invitation_process;
	p2p.invitation_received = wpas_invitation_received;
	p2p.invitation_result = wpas_invitation_result;
	p2p.get_noa = wpas_get_noa;
	p2p.go_connected = wpas_go_connected;
	p2p.presence_resp = wpas_presence_resp;
	p2p.is_concurrent_session_active = wpas_is_concurrent_session_active;
	p2p.is_p2p_in_progress = _wpas_p2p_in_progress;

	os_memcpy(wpa_s->global->p2p_dev_addr, wpa_s->own_addr, ETH_ALEN);
	os_memcpy(p2p.dev_addr, wpa_s->global->p2p_dev_addr, ETH_ALEN);
	p2p.dev_name = wpa_s->conf->device_name;
	p2p.manufacturer = wpa_s->conf->manufacturer;
	p2p.model_name = wpa_s->conf->model_name;
	p2p.model_number = wpa_s->conf->model_number;
	p2p.serial_number = wpa_s->conf->serial_number;
	if (wpa_s->wps) {
		os_memcpy(p2p.uuid, wpa_s->wps->uuid, 16);
		p2p.config_methods = wpa_s->wps->config_methods;
	}

	if (wpas_p2p_setup_channels(wpa_s, &p2p.channels, &p2p.cli_channels)) {
		wpa_printf(MSG_ERROR,
			   "P2P: Failed to configure supported channel list");
		return -1;
	}

	if (wpa_s->conf->p2p_listen_reg_class &&
	    wpa_s->conf->p2p_listen_channel) {
		p2p.reg_class = wpa_s->conf->p2p_listen_reg_class;
		p2p.channel = wpa_s->conf->p2p_listen_channel;
		p2p.channel_forced = 1;
	} else {
		/*
		 * Pick one of the social channels randomly as the listen
		 * channel.
		 */
		if (p2p_config_get_random_social(&p2p, &p2p.reg_class,
						 &p2p.channel) != 0) {
			wpa_printf(MSG_ERROR,
				   "P2P: Failed to select random social channel as listen channel");
			return -1;
		}
		p2p.channel_forced = 0;
	}
	wpa_printf(MSG_DEBUG, "P2P: Own listen channel: %d:%d",
		   p2p.reg_class, p2p.channel);

	if (wpa_s->conf->p2p_oper_reg_class &&
	    wpa_s->conf->p2p_oper_channel) {
		p2p.op_reg_class = wpa_s->conf->p2p_oper_reg_class;
		p2p.op_channel = wpa_s->conf->p2p_oper_channel;
		p2p.cfg_op_channel = 1;
		wpa_printf(MSG_DEBUG, "P2P: Configured operating channel: "
			   "%d:%d", p2p.op_reg_class, p2p.op_channel);

	} else {
		/*
		 * Use random operation channel from 2.4 GHz band social
		 * channels (1, 6, 11) or band 60 GHz social channel (2) if no
		 * other preference is indicated.
		 */
		if (p2p_config_get_random_social(&p2p, &p2p.op_reg_class,
						 &p2p.op_channel) != 0) {
			wpa_printf(MSG_ERROR,
				   "P2P: Failed to select random social channel as operation channel");
			return -1;
		}
		p2p.cfg_op_channel = 0;
		wpa_printf(MSG_DEBUG, "P2P: Random operating channel: "
			   "%d:%d", p2p.op_reg_class, p2p.op_channel);
	}

	if (wpa_s->conf->p2p_pref_chan && wpa_s->conf->num_p2p_pref_chan) {
		p2p.pref_chan = wpa_s->conf->p2p_pref_chan;
		p2p.num_pref_chan = wpa_s->conf->num_p2p_pref_chan;
	}

	if (wpa_s->conf->country[0] && wpa_s->conf->country[1]) {
		os_memcpy(p2p.country, wpa_s->conf->country, 2);
		p2p.country[2] = 0x04;
	} else
		os_memcpy(p2p.country, "XX\x04", 3);

	os_memcpy(p2p.pri_dev_type, wpa_s->conf->device_type,
		  WPS_DEV_TYPE_LEN);

	p2p.num_sec_dev_types = wpa_s->conf->num_sec_device_types;
	os_memcpy(p2p.sec_dev_type, wpa_s->conf->sec_device_type,
		  p2p.num_sec_dev_types * WPS_DEV_TYPE_LEN);

	p2p.concurrent_operations = !!(wpa_s->drv_flags &
				       WPA_DRIVER_FLAGS_P2P_CONCURRENT);

	p2p.max_peers = 100;

	if (wpa_s->conf->p2p_ssid_postfix) {
		p2p.ssid_postfix_len =
			os_strlen(wpa_s->conf->p2p_ssid_postfix);
		if (p2p.ssid_postfix_len > sizeof(p2p.ssid_postfix))
			p2p.ssid_postfix_len = sizeof(p2p.ssid_postfix);
		os_memcpy(p2p.ssid_postfix, wpa_s->conf->p2p_ssid_postfix,
			  p2p.ssid_postfix_len);
	}

	p2p.p2p_intra_bss = wpa_s->conf->p2p_intra_bss;

	p2p.max_listen = wpa_s->max_remain_on_chan;

	if (wpa_s->conf->p2p_passphrase_len >= 8 &&
	    wpa_s->conf->p2p_passphrase_len <= 63)
		p2p.passphrase_len = wpa_s->conf->p2p_passphrase_len;
	else
		p2p.passphrase_len = 8;

	global->p2p = p2p_init(&p2p);
	if (global->p2p == NULL)
		return -1;
	global->p2p_init_wpa_s = wpa_s;

	for (i = 0; i < MAX_WPS_VENDOR_EXT; i++) {
		if (wpa_s->conf->wps_vendor_ext[i] == NULL)
			continue;
		p2p_add_wps_vendor_extension(
			global->p2p, wpa_s->conf->wps_vendor_ext[i]);
	}

	p2p_set_no_go_freq(global->p2p, &wpa_s->conf->p2p_no_go_freq);

	return 0;
}


/**
 * wpas_p2p_deinit - Deinitialize per-interface P2P data
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 *
 * This function deinitialize per-interface P2P data.
 */
void wpas_p2p_deinit(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->driver && wpa_s->drv_priv)
		wpa_drv_probe_req_report(wpa_s, 0);

	if (wpa_s->go_params) {
		/* Clear any stored provisioning info */
		p2p_clear_provisioning_info(
			wpa_s->global->p2p,
			wpa_s->go_params->peer_device_addr);
	}

	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;
	eloop_cancel_timeout(wpas_p2p_psk_failure_removal, wpa_s, NULL);
	eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s, NULL);
	eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
	wpa_s->p2p_long_listen = 0;
	eloop_cancel_timeout(wpas_p2p_long_listen_timeout, wpa_s, NULL);
	eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL);
	wpas_p2p_remove_pending_group_interface(wpa_s);
	eloop_cancel_timeout(wpas_p2p_group_freq_conflict, wpa_s, NULL);
	wpas_p2p_listen_work_done(wpa_s);
	if (wpa_s->p2p_send_action_work) {
		os_free(wpa_s->p2p_send_action_work->ctx);
		radio_work_done(wpa_s->p2p_send_action_work);
		wpa_s->p2p_send_action_work = NULL;
	}
	eloop_cancel_timeout(wpas_p2p_send_action_work_timeout, wpa_s, NULL);

	wpabuf_free(wpa_s->p2p_oob_dev_pw);
	wpa_s->p2p_oob_dev_pw = NULL;

	/* TODO: remove group interface from the driver if this wpa_s instance
	 * is on top of a P2P group interface */
}


/**
 * wpas_p2p_deinit_global - Deinitialize global P2P module
 * @global: Pointer to global data from wpa_supplicant_init()
 *
 * This function deinitializes the global (per device) P2P module.
 */
static void wpas_p2p_deinit_global(struct wpa_global *global)
{
	struct wpa_supplicant *wpa_s, *tmp;

	wpa_s = global->ifaces;

	wpas_p2p_service_flush(global->p2p_init_wpa_s);

	/* Remove remaining P2P group interfaces */
	while (wpa_s && wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE)
		wpa_s = wpa_s->next;
	while (wpa_s) {
		tmp = global->ifaces;
		while (tmp &&
		       (tmp == wpa_s ||
			tmp->p2p_group_interface == NOT_P2P_GROUP_INTERFACE)) {
			tmp = tmp->next;
		}
		if (tmp == NULL)
			break;
		/* Disconnect from the P2P group and deinit the interface */
		wpas_p2p_disconnect(tmp);
	}

	/*
	 * Deinit GO data on any possibly remaining interface (if main
	 * interface is used as GO).
	 */
	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s->ap_iface)
			wpas_p2p_group_deinit(wpa_s);
	}

	p2p_deinit(global->p2p);
	global->p2p = NULL;
	global->p2p_init_wpa_s = NULL;
}


static int wpas_p2p_create_iface(struct wpa_supplicant *wpa_s)
{
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
	    wpa_s->conf->p2p_no_group_iface)
		return 0; /* separate interface disabled per configuration */
	if (wpa_s->drv_flags &
	    (WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE |
	     WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P))
		return 1; /* P2P group requires a new interface in every case
			   */
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CONCURRENT))
		return 0; /* driver does not support concurrent operations */
	if (wpa_s->global->ifaces->next)
		return 1; /* more that one interface already in use */
	if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
		return 1; /* this interface is already in use */
	return 0;
}


static int wpas_p2p_start_go_neg(struct wpa_supplicant *wpa_s,
				 const u8 *peer_addr,
				 enum p2p_wps_method wps_method,
				 int go_intent, const u8 *own_interface_addr,
				 unsigned int force_freq, int persistent_group,
				 struct wpa_ssid *ssid, unsigned int pref_freq)
{
	if (persistent_group && wpa_s->conf->persistent_reconnect)
		persistent_group = 2;

	/*
	 * Increase GO config timeout if HT40 is used since it takes some time
	 * to scan channels for coex purposes before the BSS can be started.
	 */
	p2p_set_config_timeout(wpa_s->global->p2p,
			       wpa_s->p2p_go_ht40 ? 255 : 100, 20);

	return p2p_connect(wpa_s->global->p2p, peer_addr, wps_method,
			   go_intent, own_interface_addr, force_freq,
			   persistent_group, ssid ? ssid->ssid : NULL,
			   ssid ? ssid->ssid_len : 0,
			   wpa_s->p2p_pd_before_go_neg, pref_freq,
			   wps_method == WPS_NFC ? wpa_s->p2p_oob_dev_pw_id :
			   0);
}


static int wpas_p2p_auth_go_neg(struct wpa_supplicant *wpa_s,
				const u8 *peer_addr,
				enum p2p_wps_method wps_method,
				int go_intent, const u8 *own_interface_addr,
				unsigned int force_freq, int persistent_group,
				struct wpa_ssid *ssid, unsigned int pref_freq)
{
	if (persistent_group && wpa_s->conf->persistent_reconnect)
		persistent_group = 2;

	return p2p_authorize(wpa_s->global->p2p, peer_addr, wps_method,
			     go_intent, own_interface_addr, force_freq,
			     persistent_group, ssid ? ssid->ssid : NULL,
			     ssid ? ssid->ssid_len : 0, pref_freq,
			     wps_method == WPS_NFC ? wpa_s->p2p_oob_dev_pw_id :
			     0);
}


static void wpas_p2p_check_join_scan_limit(struct wpa_supplicant *wpa_s)
{
	wpa_s->p2p_join_scan_count++;
	wpa_printf(MSG_DEBUG, "P2P: Join scan attempt %d",
		   wpa_s->p2p_join_scan_count);
	if (wpa_s->p2p_join_scan_count > P2P_MAX_JOIN_SCAN_ATTEMPTS) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to find GO " MACSTR
			   " for join operationg - stop join attempt",
			   MAC2STR(wpa_s->pending_join_iface_addr));
		eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
		if (wpa_s->p2p_auto_pd) {
			wpa_s->p2p_auto_pd = 0;
			wpa_msg_global(wpa_s, MSG_INFO,
				       P2P_EVENT_PROV_DISC_FAILURE
				       " p2p_dev_addr=" MACSTR " status=N/A",
				       MAC2STR(wpa_s->pending_join_dev_addr));
			return;
		}
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_GROUP_FORMATION_FAILURE);
	}
}


static int wpas_check_freq_conflict(struct wpa_supplicant *wpa_s, int freq)
{
	int res;
	unsigned int num, i;
	struct wpa_used_freq_data *freqs;

	if (wpas_p2p_num_unused_channels(wpa_s) > 0) {
		/* Multiple channels are supported and not all are in use */
		return 0;
	}

	freqs = os_calloc(wpa_s->num_multichan_concurrent,
			  sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return 1;

	num = wpas_p2p_valid_oper_freqs(wpa_s, freqs,
					wpa_s->num_multichan_concurrent);

	for (i = 0; i < num; i++) {
		if (freqs[i].freq == freq) {
			wpa_printf(MSG_DEBUG, "P2P: Frequency %d MHz in use by another virtual interface and can be used",
				   freq);
			res = 0;
			goto exit_free;
		}
	}

	wpa_printf(MSG_DEBUG, "P2P: No valid operating frequencies");
	res = 1;

exit_free:
	os_free(freqs);
	return res;
}


static int wpas_p2p_peer_go(struct wpa_supplicant *wpa_s,
			    const u8 *peer_dev_addr)
{
	struct wpa_bss *bss;
	int updated;

	bss = wpa_bss_get_p2p_dev_addr(wpa_s, peer_dev_addr);
	if (bss == NULL)
		return -1;
	if (bss->last_update_idx < wpa_s->bss_update_idx) {
		wpa_printf(MSG_DEBUG, "P2P: Peer BSS entry not updated in the "
			   "last scan");
		return 0;
	}

	updated = os_reltime_before(&wpa_s->p2p_auto_started,
				    &bss->last_update);
	wpa_printf(MSG_DEBUG, "P2P: Current BSS entry for peer updated at "
		   "%ld.%06ld (%supdated in last scan)",
		   bss->last_update.sec, bss->last_update.usec,
		   updated ? "": "not ");

	return updated;
}


static void wpas_p2p_scan_res_join(struct wpa_supplicant *wpa_s,
				   struct wpa_scan_results *scan_res)
{
	struct wpa_bss *bss = NULL;
	int freq;
	u8 iface_addr[ETH_ALEN];

	eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);

	if (wpa_s->global->p2p_disabled)
		return;

	wpa_printf(MSG_DEBUG, "P2P: Scan results received (%d BSS) for %sjoin",
		   scan_res ? (int) scan_res->num : -1,
		   wpa_s->p2p_auto_join ? "auto_" : "");

	if (scan_res)
		wpas_p2p_scan_res_handler(wpa_s, scan_res);

	if (wpa_s->p2p_auto_pd) {
		int join = wpas_p2p_peer_go(wpa_s,
					    wpa_s->pending_join_dev_addr);
		if (join == 0 &&
		    wpa_s->auto_pd_scan_retry < P2P_AUTO_PD_SCAN_ATTEMPTS) {
			wpa_s->auto_pd_scan_retry++;
			bss = wpa_bss_get_bssid_latest(
				wpa_s, wpa_s->pending_join_dev_addr);
			if (bss) {
				freq = bss->freq;
				wpa_printf(MSG_DEBUG, "P2P: Scan retry %d for "
					   "the peer " MACSTR " at %d MHz",
					   wpa_s->auto_pd_scan_retry,
					   MAC2STR(wpa_s->
						   pending_join_dev_addr),
					   freq);
				wpas_p2p_join_scan_req(wpa_s, freq, NULL, 0);
				return;
			}
		}

		if (join < 0)
			join = 0;

		wpa_s->p2p_auto_pd = 0;
		wpa_s->pending_pd_use = join ? AUTO_PD_JOIN : AUTO_PD_GO_NEG;
		wpa_printf(MSG_DEBUG, "P2P: Auto PD with " MACSTR " join=%d",
			   MAC2STR(wpa_s->pending_join_dev_addr), join);
		if (p2p_prov_disc_req(wpa_s->global->p2p,
				      wpa_s->pending_join_dev_addr,
				      wpa_s->pending_pd_config_methods, join,
				      0, wpa_s->user_initiated_pd) < 0) {
			wpa_s->p2p_auto_pd = 0;
			wpa_msg_global(wpa_s, MSG_INFO,
				       P2P_EVENT_PROV_DISC_FAILURE
				       " p2p_dev_addr=" MACSTR " status=N/A",
				       MAC2STR(wpa_s->pending_join_dev_addr));
		}
		return;
	}

	if (wpa_s->p2p_auto_join) {
		int join = wpas_p2p_peer_go(wpa_s,
					    wpa_s->pending_join_dev_addr);
		if (join < 0) {
			wpa_printf(MSG_DEBUG, "P2P: Peer was not found to be "
				   "running a GO -> use GO Negotiation");
			wpas_p2p_connect(wpa_s, wpa_s->pending_join_dev_addr,
					 wpa_s->p2p_pin, wpa_s->p2p_wps_method,
					 wpa_s->p2p_persistent_group, 0, 0, 0,
					 wpa_s->p2p_go_intent,
					 wpa_s->p2p_connect_freq,
					 wpa_s->p2p_persistent_id,
					 wpa_s->p2p_pd_before_go_neg,
					 wpa_s->p2p_go_ht40,
					 wpa_s->p2p_go_vht);
			return;
		}

		wpa_printf(MSG_DEBUG, "P2P: Peer was found running GO%s -> "
			   "try to join the group", join ? "" :
			   " in older scan");
		if (!join)
			wpa_s->p2p_fallback_to_go_neg = 1;
	}

	freq = p2p_get_oper_freq(wpa_s->global->p2p,
				 wpa_s->pending_join_iface_addr);
	if (freq < 0 &&
	    p2p_get_interface_addr(wpa_s->global->p2p,
				   wpa_s->pending_join_dev_addr,
				   iface_addr) == 0 &&
	    os_memcmp(iface_addr, wpa_s->pending_join_dev_addr, ETH_ALEN) != 0
	    && !wpa_bss_get_bssid(wpa_s, wpa_s->pending_join_iface_addr)) {
		wpa_printf(MSG_DEBUG, "P2P: Overwrite pending interface "
			   "address for join from " MACSTR " to " MACSTR
			   " based on newly discovered P2P peer entry",
			   MAC2STR(wpa_s->pending_join_iface_addr),
			   MAC2STR(iface_addr));
		os_memcpy(wpa_s->pending_join_iface_addr, iface_addr,
			  ETH_ALEN);

		freq = p2p_get_oper_freq(wpa_s->global->p2p,
					 wpa_s->pending_join_iface_addr);
	}
	if (freq >= 0) {
		wpa_printf(MSG_DEBUG, "P2P: Target GO operating frequency "
			   "from P2P peer table: %d MHz", freq);
	}
	if (wpa_s->p2p_join_ssid_len) {
		wpa_printf(MSG_DEBUG, "P2P: Trying to find target GO BSS entry based on BSSID "
			   MACSTR " and SSID %s",
			   MAC2STR(wpa_s->pending_join_iface_addr),
			   wpa_ssid_txt(wpa_s->p2p_join_ssid,
					wpa_s->p2p_join_ssid_len));
		bss = wpa_bss_get(wpa_s, wpa_s->pending_join_iface_addr,
				  wpa_s->p2p_join_ssid,
				  wpa_s->p2p_join_ssid_len);
	}
	if (!bss) {
		wpa_printf(MSG_DEBUG, "P2P: Trying to find target GO BSS entry based on BSSID "
			   MACSTR, MAC2STR(wpa_s->pending_join_iface_addr));
		bss = wpa_bss_get_bssid_latest(wpa_s,
					       wpa_s->pending_join_iface_addr);
	}
	if (bss) {
		freq = bss->freq;
		wpa_printf(MSG_DEBUG, "P2P: Target GO operating frequency "
			   "from BSS table: %d MHz (SSID %s)", freq,
			   wpa_ssid_txt(bss->ssid, bss->ssid_len));
	}
	if (freq > 0) {
		u16 method;

		if (wpas_check_freq_conflict(wpa_s, freq) > 0) {
			wpa_msg_global(wpa_s->parent, MSG_INFO,
				       P2P_EVENT_GROUP_FORMATION_FAILURE
				       "reason=FREQ_CONFLICT");
			return;
		}

		wpa_printf(MSG_DEBUG, "P2P: Send Provision Discovery Request "
			   "prior to joining an existing group (GO " MACSTR
			   " freq=%u MHz)",
			   MAC2STR(wpa_s->pending_join_dev_addr), freq);
		wpa_s->pending_pd_before_join = 1;

		switch (wpa_s->pending_join_wps_method) {
		case WPS_PIN_DISPLAY:
			method = WPS_CONFIG_KEYPAD;
			break;
		case WPS_PIN_KEYPAD:
			method = WPS_CONFIG_DISPLAY;
			break;
		case WPS_PBC:
			method = WPS_CONFIG_PUSHBUTTON;
			break;
		default:
			method = 0;
			break;
		}

		if ((p2p_get_provisioning_info(wpa_s->global->p2p,
					       wpa_s->pending_join_dev_addr) ==
		     method)) {
			/*
			 * We have already performed provision discovery for
			 * joining the group. Proceed directly to join
			 * operation without duplicated provision discovery. */
			wpa_printf(MSG_DEBUG, "P2P: Provision discovery "
				   "with " MACSTR " already done - proceed to "
				   "join",
				   MAC2STR(wpa_s->pending_join_dev_addr));
			wpa_s->pending_pd_before_join = 0;
			goto start;
		}

		if (p2p_prov_disc_req(wpa_s->global->p2p,
				      wpa_s->pending_join_dev_addr, method, 1,
				      freq, wpa_s->user_initiated_pd) < 0) {
			wpa_printf(MSG_DEBUG, "P2P: Failed to send Provision "
				   "Discovery Request before joining an "
				   "existing group");
			wpa_s->pending_pd_before_join = 0;
			goto start;
		}
		return;
	}

	wpa_printf(MSG_DEBUG, "P2P: Failed to find BSS/GO - try again later");
	eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
	eloop_register_timeout(1, 0, wpas_p2p_join_scan, wpa_s, NULL);
	wpas_p2p_check_join_scan_limit(wpa_s);
	return;

start:
	/* Start join operation immediately */
	wpas_p2p_join_start(wpa_s, 0, NULL, 0);
}


static void wpas_p2p_join_scan_req(struct wpa_supplicant *wpa_s, int freq,
				   const u8 *ssid, size_t ssid_len)
{
	int ret;
	struct wpa_driver_scan_params params;
	struct wpabuf *wps_ie, *ies;
	size_t ielen;
	int freqs[2] = { 0, 0 };

	os_memset(&params, 0, sizeof(params));

	/* P2P Wildcard SSID */
	params.num_ssids = 1;
	if (ssid && ssid_len) {
		params.ssids[0].ssid = ssid;
		params.ssids[0].ssid_len = ssid_len;
		os_memcpy(wpa_s->p2p_join_ssid, ssid, ssid_len);
		wpa_s->p2p_join_ssid_len = ssid_len;
	} else {
		params.ssids[0].ssid = (u8 *) P2P_WILDCARD_SSID;
		params.ssids[0].ssid_len = P2P_WILDCARD_SSID_LEN;
		wpa_s->p2p_join_ssid_len = 0;
	}

	wpa_s->wps->dev.p2p = 1;
	wps_ie = wps_build_probe_req_ie(DEV_PW_DEFAULT, &wpa_s->wps->dev,
					wpa_s->wps->uuid, WPS_REQ_ENROLLEE, 0,
					NULL);
	if (wps_ie == NULL) {
		wpas_p2p_scan_res_join(wpa_s, NULL);
		return;
	}

	ielen = p2p_scan_ie_buf_len(wpa_s->global->p2p);
	ies = wpabuf_alloc(wpabuf_len(wps_ie) + ielen);
	if (ies == NULL) {
		wpabuf_free(wps_ie);
		wpas_p2p_scan_res_join(wpa_s, NULL);
		return;
	}
	wpabuf_put_buf(ies, wps_ie);
	wpabuf_free(wps_ie);

	p2p_scan_ie(wpa_s->global->p2p, ies, NULL);

	params.p2p_probe = 1;
	params.extra_ies = wpabuf_head(ies);
	params.extra_ies_len = wpabuf_len(ies);

	if (!freq) {
		int oper_freq;
		/*
		 * If freq is not provided, check the operating freq of the GO
		 * and use a single channel scan on if possible.
		 */
		oper_freq = p2p_get_oper_freq(wpa_s->global->p2p,
					      wpa_s->pending_join_iface_addr);
		if (oper_freq > 0)
			freq = oper_freq;
	}
	if (freq > 0) {
		freqs[0] = freq;
		params.freqs = freqs;
	}

	/*
	 * Run a scan to update BSS table and start Provision Discovery once
	 * the new scan results become available.
	 */
	ret = wpa_drv_scan(wpa_s, &params);
	if (!ret) {
		os_get_reltime(&wpa_s->scan_trigger_time);
		wpa_s->scan_res_handler = wpas_p2p_scan_res_join;
		wpa_s->own_scan_requested = 1;
	}

	wpabuf_free(ies);

	if (ret) {
		wpa_printf(MSG_DEBUG, "P2P: Failed to start scan for join - "
			   "try again later");
		eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
		eloop_register_timeout(1, 0, wpas_p2p_join_scan, wpa_s, NULL);
		wpas_p2p_check_join_scan_limit(wpa_s);
	}
}


static void wpas_p2p_join_scan(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_join_scan_req(wpa_s, 0, NULL, 0);
}


static int wpas_p2p_join(struct wpa_supplicant *wpa_s, const u8 *iface_addr,
			 const u8 *dev_addr, enum p2p_wps_method wps_method,
			 int auto_join, int op_freq,
			 const u8 *ssid, size_t ssid_len)
{
	wpa_printf(MSG_DEBUG, "P2P: Request to join existing group (iface "
		   MACSTR " dev " MACSTR " op_freq=%d)%s",
		   MAC2STR(iface_addr), MAC2STR(dev_addr), op_freq,
		   auto_join ? " (auto_join)" : "");
	if (ssid && ssid_len) {
		wpa_printf(MSG_DEBUG, "P2P: Group SSID specified: %s",
			   wpa_ssid_txt(ssid, ssid_len));
	}

	wpa_s->p2p_auto_pd = 0;
	wpa_s->p2p_auto_join = !!auto_join;
	os_memcpy(wpa_s->pending_join_iface_addr, iface_addr, ETH_ALEN);
	os_memcpy(wpa_s->pending_join_dev_addr, dev_addr, ETH_ALEN);
	wpa_s->pending_join_wps_method = wps_method;

	/* Make sure we are not running find during connection establishment */
	wpas_p2p_stop_find(wpa_s);

	wpa_s->p2p_join_scan_count = 0;
	wpas_p2p_join_scan_req(wpa_s, op_freq, ssid, ssid_len);
	return 0;
}


static int wpas_p2p_join_start(struct wpa_supplicant *wpa_s, int freq,
			       const u8 *ssid, size_t ssid_len)
{
	struct wpa_supplicant *group;
	struct p2p_go_neg_results res;
	struct wpa_bss *bss;

	group = wpas_p2p_get_group_iface(wpa_s, 0, 0);
	if (group == NULL)
		return -1;
	if (group != wpa_s) {
		os_memcpy(group->p2p_pin, wpa_s->p2p_pin,
			  sizeof(group->p2p_pin));
		group->p2p_wps_method = wpa_s->p2p_wps_method;
	} else {
		/*
		 * Need to mark the current interface for p2p_group_formation
		 * when a separate group interface is not used. This is needed
		 * to allow p2p_cancel stop a pending p2p_connect-join.
		 * wpas_p2p_init_group_interface() addresses this for the case
		 * where a separate group interface is used.
		 */
		wpa_s->global->p2p_group_formation = wpa_s;
	}

	group->p2p_in_provisioning = 1;
	group->p2p_fallback_to_go_neg = wpa_s->p2p_fallback_to_go_neg;

	os_memset(&res, 0, sizeof(res));
	os_memcpy(res.peer_device_addr, wpa_s->pending_join_dev_addr, ETH_ALEN);
	os_memcpy(res.peer_interface_addr, wpa_s->pending_join_iface_addr,
		  ETH_ALEN);
	res.wps_method = wpa_s->pending_join_wps_method;
	if (freq && ssid && ssid_len) {
		res.freq = freq;
		res.ssid_len = ssid_len;
		os_memcpy(res.ssid, ssid, ssid_len);
	} else {
		bss = wpa_bss_get_bssid_latest(wpa_s,
					       wpa_s->pending_join_iface_addr);
		if (bss) {
			res.freq = bss->freq;
			res.ssid_len = bss->ssid_len;
			os_memcpy(res.ssid, bss->ssid, bss->ssid_len);
			wpa_printf(MSG_DEBUG, "P2P: Join target GO operating frequency from BSS table: %d MHz (SSID %s)",
				   bss->freq,
				   wpa_ssid_txt(bss->ssid, bss->ssid_len));
		}
	}

	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_printf(MSG_DEBUG, "P2P: Cancel remain-on-channel prior to "
			   "starting client");
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}
	wpas_start_wps_enrollee(group, &res);

	/*
	 * Allow a longer timeout for join-a-running-group than normal 15
	 * second group formation timeout since the GO may not have authorized
	 * our connection yet.
	 */
	eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s, NULL);
	eloop_register_timeout(60, 0, wpas_p2p_group_formation_timeout,
			       wpa_s, NULL);

	return 0;
}


static int wpas_p2p_setup_freqs(struct wpa_supplicant *wpa_s, int freq,
				int *force_freq, int *pref_freq, int go)
{
	struct wpa_used_freq_data *freqs;
	int res, best_freq, num_unused;
	unsigned int freq_in_use = 0, num, i;

	freqs = os_calloc(wpa_s->num_multichan_concurrent,
			  sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return -1;

	num = wpas_p2p_valid_oper_freqs(wpa_s, freqs,
					wpa_s->num_multichan_concurrent);

	/*
	 * It is possible that the total number of used frequencies is bigger
	 * than the number of frequencies used for P2P, so get the system wide
	 * number of unused frequencies.
	 */
	num_unused = wpas_p2p_num_unused_channels(wpa_s);

	wpa_printf(MSG_DEBUG,
		   "P2P: Setup freqs: freq=%d num_MCC=%d shared_freqs=%u num_unused=%d",
		   freq, wpa_s->num_multichan_concurrent, num, num_unused);

	if (freq > 0) {
		int ret;
		if (go)
			ret = p2p_supported_freq(wpa_s->global->p2p, freq);
		else
			ret = p2p_supported_freq_cli(wpa_s->global->p2p, freq);
		if (!ret) {
			wpa_printf(MSG_DEBUG, "P2P: The forced channel "
				   "(%u MHz) is not supported for P2P uses",
				   freq);
			res = -3;
			goto exit_free;
		}

		for (i = 0; i < num; i++) {
			if (freqs[i].freq == freq)
				freq_in_use = 1;
		}

		if (num_unused <= 0 && !freq_in_use) {
			wpa_printf(MSG_DEBUG, "P2P: Cannot start P2P group on %u MHz as there are no available channels",
				   freq);
			res = -2;
			goto exit_free;
		}
		wpa_printf(MSG_DEBUG, "P2P: Trying to force us to use the "
			   "requested channel (%u MHz)", freq);
		*force_freq = freq;
		goto exit_ok;
	}

	best_freq = wpas_p2p_pick_best_used_freq(wpa_s, freqs, num);

	/* We have a candidate frequency to use */
	if (best_freq > 0) {
		if (*pref_freq == 0 && num_unused > 0) {
			wpa_printf(MSG_DEBUG, "P2P: Try to prefer a frequency (%u MHz) we are already using",
				   best_freq);
			*pref_freq = best_freq;
		} else {
			wpa_printf(MSG_DEBUG, "P2P: Try to force us to use frequency (%u MHz) which is already in use",
				   best_freq);
			*force_freq = best_freq;
		}
	} else if (num_unused > 0) {
		wpa_printf(MSG_DEBUG,
			   "P2P: Current operating channels are not available for P2P. Try to use another channel");
		*force_freq = 0;
	} else {
		wpa_printf(MSG_DEBUG,
			   "P2P: All channels are in use and none of them are P2P enabled. Cannot start P2P group");
		res = -2;
		goto exit_free;
	}

exit_ok:
	res = 0;
exit_free:
	os_free(freqs);
	return res;
}


/**
 * wpas_p2p_connect - Request P2P Group Formation to be started
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * @peer_addr: Address of the peer P2P Device
 * @pin: PIN to use during provisioning or %NULL to indicate PBC mode
 * @persistent_group: Whether to create a persistent group
 * @auto_join: Whether to select join vs. GO Negotiation automatically
 * @join: Whether to join an existing group (as a client) instead of starting
 *	Group Owner negotiation; @peer_addr is BSSID in that case
 * @auth: Whether to only authorize the connection instead of doing that and
 *	initiating Group Owner negotiation
 * @go_intent: GO Intent or -1 to use default
 * @freq: Frequency for the group or 0 for auto-selection
 * @persistent_id: Persistent group credentials to use for forcing GO
 *	parameters or -1 to generate new values (SSID/passphrase)
 * @pd: Whether to send Provision Discovery prior to GO Negotiation as an
 *	interoperability workaround when initiating group formation
 * @ht40: Start GO with 40 MHz channel width
 * @vht:  Start GO with VHT support
 * Returns: 0 or new PIN (if pin was %NULL) on success, -1 on unspecified
 *	failure, -2 on failure due to channel not currently available,
 *	-3 if forced channel is not supported
 */
int wpas_p2p_connect(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
		     const char *pin, enum p2p_wps_method wps_method,
		     int persistent_group, int auto_join, int join, int auth,
		     int go_intent, int freq, int persistent_id, int pd,
		     int ht40, int vht)
{
	int force_freq = 0, pref_freq = 0;
	int ret = 0, res;
	enum wpa_driver_if_type iftype;
	const u8 *if_addr;
	struct wpa_ssid *ssid = NULL;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (persistent_id >= 0) {
		ssid = wpa_config_get_network(wpa_s->conf, persistent_id);
		if (ssid == NULL || ssid->disabled != 2 ||
		    ssid->mode != WPAS_MODE_P2P_GO)
			return -1;
	}

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	wpa_s->global->p2p_fail_on_wps_complete = 0;

	if (go_intent < 0)
		go_intent = wpa_s->conf->p2p_go_intent;

	if (!auth)
		wpa_s->p2p_long_listen = 0;

	wpa_s->p2p_wps_method = wps_method;
	wpa_s->p2p_persistent_group = !!persistent_group;
	wpa_s->p2p_persistent_id = persistent_id;
	wpa_s->p2p_go_intent = go_intent;
	wpa_s->p2p_connect_freq = freq;
	wpa_s->p2p_fallback_to_go_neg = 0;
	wpa_s->p2p_pd_before_go_neg = !!pd;
	wpa_s->p2p_go_ht40 = !!ht40;
	wpa_s->p2p_go_vht = !!vht;

	if (pin)
		os_strlcpy(wpa_s->p2p_pin, pin, sizeof(wpa_s->p2p_pin));
	else if (wps_method == WPS_PIN_DISPLAY) {
		ret = wps_generate_pin();
		os_snprintf(wpa_s->p2p_pin, sizeof(wpa_s->p2p_pin), "%08d",
			    ret);
		wpa_printf(MSG_DEBUG, "P2P: Randomly generated PIN: %s",
			   wpa_s->p2p_pin);
	} else
		wpa_s->p2p_pin[0] = '\0';

	if (join || auto_join) {
		u8 iface_addr[ETH_ALEN], dev_addr[ETH_ALEN];
		if (auth) {
			wpa_printf(MSG_DEBUG, "P2P: Authorize invitation to "
				   "connect a running group from " MACSTR,
				   MAC2STR(peer_addr));
			os_memcpy(wpa_s->p2p_auth_invite, peer_addr, ETH_ALEN);
			return ret;
		}
		os_memcpy(dev_addr, peer_addr, ETH_ALEN);
		if (p2p_get_interface_addr(wpa_s->global->p2p, peer_addr,
					   iface_addr) < 0) {
			os_memcpy(iface_addr, peer_addr, ETH_ALEN);
			p2p_get_dev_addr(wpa_s->global->p2p, peer_addr,
					 dev_addr);
		}
		if (auto_join) {
			os_get_reltime(&wpa_s->p2p_auto_started);
			wpa_printf(MSG_DEBUG, "P2P: Auto join started at "
				   "%ld.%06ld",
				   wpa_s->p2p_auto_started.sec,
				   wpa_s->p2p_auto_started.usec);
		}
		wpa_s->user_initiated_pd = 1;
		if (wpas_p2p_join(wpa_s, iface_addr, dev_addr, wps_method,
				  auto_join, freq, NULL, 0) < 0)
			return -1;
		return ret;
	}

	res = wpas_p2p_setup_freqs(wpa_s, freq, &force_freq, &pref_freq,
				   go_intent == 15);
	if (res)
		return res;
	wpas_p2p_set_own_freq_preference(wpa_s,
					 force_freq ? force_freq : pref_freq);

	wpa_s->create_p2p_iface = wpas_p2p_create_iface(wpa_s);

	if (wpa_s->create_p2p_iface) {
		/* Prepare to add a new interface for the group */
		iftype = WPA_IF_P2P_GROUP;
		if (go_intent == 15)
			iftype = WPA_IF_P2P_GO;
		if (wpas_p2p_add_group_interface(wpa_s, iftype) < 0) {
			wpa_printf(MSG_ERROR, "P2P: Failed to allocate a new "
				   "interface for the group");
			return -1;
		}

		if_addr = wpa_s->pending_interface_addr;
	} else
		if_addr = wpa_s->own_addr;

	if (auth) {
		if (wpas_p2p_auth_go_neg(wpa_s, peer_addr, wps_method,
					 go_intent, if_addr,
					 force_freq, persistent_group, ssid,
					 pref_freq) < 0)
			return -1;
		return ret;
	}

	if (wpas_p2p_start_go_neg(wpa_s, peer_addr, wps_method,
				  go_intent, if_addr, force_freq,
				  persistent_group, ssid, pref_freq) < 0) {
		if (wpa_s->create_p2p_iface)
			wpas_p2p_remove_pending_group_interface(wpa_s);
		return -1;
	}
	return ret;
}


/**
 * wpas_p2p_remain_on_channel_cb - Indication of remain-on-channel start
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * @freq: Frequency of the channel in MHz
 * @duration: Duration of the stay on the channel in milliseconds
 *
 * This callback is called when the driver indicates that it has started the
 * requested remain-on-channel duration.
 */
void wpas_p2p_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
				   unsigned int freq, unsigned int duration)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;
	if (wpa_s->off_channel_freq == wpa_s->pending_listen_freq) {
		p2p_listen_cb(wpa_s->global->p2p, wpa_s->pending_listen_freq,
			      wpa_s->pending_listen_duration);
		wpa_s->pending_listen_freq = 0;
	} else {
		wpa_printf(MSG_DEBUG, "P2P: Ignore remain-on-channel callback (off_channel_freq=%u pending_listen_freq=%d freq=%u duration=%u)",
			   wpa_s->off_channel_freq, wpa_s->pending_listen_freq,
			   freq, duration);
	}
}


static int wpas_p2p_listen_start(struct wpa_supplicant *wpa_s,
				 unsigned int timeout)
{
	/* Limit maximum Listen state time based on driver limitation. */
	if (timeout > wpa_s->max_remain_on_chan)
		timeout = wpa_s->max_remain_on_chan;

	return p2p_listen(wpa_s->global->p2p, timeout);
}


/**
 * wpas_p2p_cancel_remain_on_channel_cb - Remain-on-channel timeout
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * @freq: Frequency of the channel in MHz
 *
 * This callback is called when the driver indicates that a remain-on-channel
 * operation has been completed, i.e., the duration on the requested channel
 * has timed out.
 */
void wpas_p2p_cancel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
					  unsigned int freq)
{
	wpa_printf(MSG_DEBUG, "P2P: Cancel remain-on-channel callback "
		   "(p2p_long_listen=%d ms pending_action_tx=%p)",
		   wpa_s->p2p_long_listen, offchannel_pending_action_tx(wpa_s));
	wpas_p2p_listen_work_done(wpa_s);
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;
	if (p2p_listen_end(wpa_s->global->p2p, freq) > 0)
		return; /* P2P module started a new operation */
	if (offchannel_pending_action_tx(wpa_s))
		return;
	if (wpa_s->p2p_long_listen > 0)
		wpa_s->p2p_long_listen -= wpa_s->max_remain_on_chan;
	if (wpa_s->p2p_long_listen > 0) {
		wpa_printf(MSG_DEBUG, "P2P: Continuing long Listen state");
		wpas_p2p_listen_start(wpa_s, wpa_s->p2p_long_listen);
	} else {
		/*
		 * When listen duration is over, stop listen & update p2p_state
		 * to IDLE.
		 */
		p2p_stop_listen(wpa_s->global->p2p);
	}
}


/**
 * wpas_p2p_group_remove - Remove a P2P group
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * @ifname: Network interface name of the group interface or "*" to remove all
 *	groups
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to remove a P2P group. This can be used to disconnect
 * from a group in which the local end is a P2P Client or to end a P2P Group in
 * case the local end is the Group Owner. If a virtual network interface was
 * created for this group, that interface will be removed. Otherwise, only the
 * configured P2P group network will be removed from the interface.
 */
int wpas_p2p_group_remove(struct wpa_supplicant *wpa_s, const char *ifname)
{
	struct wpa_global *global = wpa_s->global;

	if (os_strcmp(ifname, "*") == 0) {
		struct wpa_supplicant *prev;
		wpa_s = global->ifaces;
		while (wpa_s) {
			prev = wpa_s;
			wpa_s = wpa_s->next;
			if (prev->p2p_group_interface !=
			    NOT_P2P_GROUP_INTERFACE ||
			    (prev->current_ssid &&
			     prev->current_ssid->p2p_group))
				wpas_p2p_disconnect(prev);
		}
		return 0;
	}

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp(wpa_s->ifname, ifname) == 0)
			break;
	}

	return wpas_p2p_disconnect(wpa_s);
}


static int wpas_p2p_select_go_freq(struct wpa_supplicant *wpa_s, int freq)
{
	unsigned int r;

	if (freq == 2) {
		wpa_printf(MSG_DEBUG, "P2P: Request to start GO on 2.4 GHz "
			   "band");
		if (wpa_s->best_24_freq > 0 &&
		    p2p_supported_freq_go(wpa_s->global->p2p,
					  wpa_s->best_24_freq)) {
			freq = wpa_s->best_24_freq;
			wpa_printf(MSG_DEBUG, "P2P: Use best 2.4 GHz band "
				   "channel: %d MHz", freq);
		} else {
			if (os_get_random((u8 *) &r, sizeof(r)) < 0)
				return -1;
			freq = 2412 + (r % 3) * 25;
			wpa_printf(MSG_DEBUG, "P2P: Use random 2.4 GHz band "
				   "channel: %d MHz", freq);
		}
	}

	if (freq == 5) {
		wpa_printf(MSG_DEBUG, "P2P: Request to start GO on 5 GHz "
			   "band");
		if (wpa_s->best_5_freq > 0 &&
		    p2p_supported_freq_go(wpa_s->global->p2p,
				       wpa_s->best_5_freq)) {
			freq = wpa_s->best_5_freq;
			wpa_printf(MSG_DEBUG, "P2P: Use best 5 GHz band "
				   "channel: %d MHz", freq);
		} else {
			if (os_get_random((u8 *) &r, sizeof(r)) < 0)
				return -1;
			freq = 5180 + (r % 4) * 20;
			if (!p2p_supported_freq_go(wpa_s->global->p2p, freq)) {
				wpa_printf(MSG_DEBUG, "P2P: Could not select "
					   "5 GHz channel for P2P group");
				return -1;
			}
			wpa_printf(MSG_DEBUG, "P2P: Use random 5 GHz band "
				   "channel: %d MHz", freq);
		}
	}

	if (freq > 0 && !p2p_supported_freq_go(wpa_s->global->p2p, freq)) {
		wpa_printf(MSG_DEBUG, "P2P: The forced channel for GO "
			   "(%u MHz) is not supported for P2P uses",
			   freq);
		return -1;
	}

	return freq;
}


static int wpas_p2p_select_freq_no_pref(struct wpa_supplicant *wpa_s,
					struct p2p_go_neg_results *params,
					const struct p2p_channels *channels)
{
	unsigned int i, r;

	/* first try some random selection of the social channels */
	if (os_get_random((u8 *) &r, sizeof(r)) < 0)
		return -1;

	for (i = 0; i < 3; i++) {
		params->freq = 2412 + ((r + i) % 3) * 25;
		if (!wpas_p2p_disallowed_freq(wpa_s->global, params->freq) &&
		    freq_included(channels, params->freq) &&
		    p2p_supported_freq(wpa_s->global->p2p, params->freq))
			goto out;
	}

	/* try all channels in reg. class 81 */
	for (i = 0; i < 11; i++) {
		params->freq = 2412 + i * 5;
		if (!wpas_p2p_disallowed_freq(wpa_s->global, params->freq) &&
		    freq_included(channels, params->freq) &&
		    p2p_supported_freq(wpa_s->global->p2p, params->freq))
			goto out;
	}

	/* try social channel class 180 channel 2 */
	params->freq = 58320 + 1 * 2160;
	if (!wpas_p2p_disallowed_freq(wpa_s->global, params->freq) &&
	    freq_included(channels, params->freq) &&
	    p2p_supported_freq(wpa_s->global->p2p, params->freq))
		goto out;

	/* try all channels in reg. class 180 */
	for (i = 0; i < 4; i++) {
		params->freq = 58320 + i * 2160;
		if (!wpas_p2p_disallowed_freq(wpa_s->global, params->freq) &&
		    freq_included(channels, params->freq) &&
		    p2p_supported_freq(wpa_s->global->p2p, params->freq))
			goto out;
	}

	wpa_printf(MSG_DEBUG, "P2P: No 2.4 and 60 GHz channel allowed");
	return -1;
out:
	wpa_printf(MSG_DEBUG, "P2P: Set GO freq %d MHz (no preference known)",
		   params->freq);
	return 0;
}


static int wpas_p2p_init_go_params(struct wpa_supplicant *wpa_s,
				   struct p2p_go_neg_results *params,
				   int freq, int ht40, int vht,
				   const struct p2p_channels *channels)
{
	struct wpa_used_freq_data *freqs;
	unsigned int pref_freq, cand_freq;
	unsigned int num, i;

	os_memset(params, 0, sizeof(*params));
	params->role_go = 1;
	params->ht40 = ht40;
	params->vht = vht;
	if (freq) {
		if (!freq_included(channels, freq)) {
			wpa_printf(MSG_DEBUG, "P2P: Forced GO freq %d MHz not "
				   "accepted", freq);
			return -1;
		}
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on forced "
			   "frequency %d MHz", freq);
		params->freq = freq;
	} else if (wpa_s->conf->p2p_oper_reg_class == 81 &&
		   wpa_s->conf->p2p_oper_channel >= 1 &&
		   wpa_s->conf->p2p_oper_channel <= 11 &&
		   freq_included(channels,
				 2407 + 5 * wpa_s->conf->p2p_oper_channel)) {
		params->freq = 2407 + 5 * wpa_s->conf->p2p_oper_channel;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on configured "
			   "frequency %d MHz", params->freq);
	} else if ((wpa_s->conf->p2p_oper_reg_class == 115 ||
		    wpa_s->conf->p2p_oper_reg_class == 116 ||
		    wpa_s->conf->p2p_oper_reg_class == 117 ||
		    wpa_s->conf->p2p_oper_reg_class == 124 ||
		    wpa_s->conf->p2p_oper_reg_class == 126 ||
		    wpa_s->conf->p2p_oper_reg_class == 127) &&
		   freq_included(channels,
				 5000 + 5 * wpa_s->conf->p2p_oper_channel)) {
		params->freq = 5000 + 5 * wpa_s->conf->p2p_oper_channel;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on configured "
			   "frequency %d MHz", params->freq);
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_overall_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_overall_freq) &&
		   freq_included(channels, wpa_s->best_overall_freq)) {
		params->freq = wpa_s->best_overall_freq;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on best overall "
			   "channel %d MHz", params->freq);
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_24_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_24_freq) &&
		   freq_included(channels, wpa_s->best_24_freq)) {
		params->freq = wpa_s->best_24_freq;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on best 2.4 GHz "
			   "channel %d MHz", params->freq);
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_5_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_5_freq) &&
		   freq_included(channels, wpa_s->best_5_freq)) {
		params->freq = wpa_s->best_5_freq;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq based on best 5 GHz "
			   "channel %d MHz", params->freq);
	} else if ((pref_freq = p2p_get_pref_freq(wpa_s->global->p2p,
						  channels))) {
		params->freq = pref_freq;
		wpa_printf(MSG_DEBUG, "P2P: Set GO freq %d MHz from preferred "
			   "channels", params->freq);
	} else {
		/* no preference, select some channel */
		if (wpas_p2p_select_freq_no_pref(wpa_s, params, channels) < 0)
			return -1;
	}

	freqs = os_calloc(wpa_s->num_multichan_concurrent,
			  sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return -1;

	num = wpas_p2p_valid_oper_freqs(wpa_s, freqs,
					wpa_s->num_multichan_concurrent);

	cand_freq = wpas_p2p_pick_best_used_freq(wpa_s, freqs, num);

	/* First try the best used frequency if possible */
	if (!freq && cand_freq > 0 && freq_included(channels, cand_freq)) {
		params->freq = cand_freq;
	} else if (!freq) {
		/* Try any of the used frequencies */
		for (i = 0; i < num; i++) {
			if (freq_included(channels, freqs[i].freq)) {
				wpa_printf(MSG_DEBUG, "P2P: Force GO on a channel we are already using (%u MHz)",
					   freqs[i].freq);
				params->freq = freqs[i].freq;
				break;
			}
		}

		if (i == num) {
			if (wpas_p2p_num_unused_channels(wpa_s) <= 0) {
				wpa_printf(MSG_DEBUG, "P2P: Cannot force GO on any of the channels we are already using");
				os_free(freqs);
				return -1;
			} else {
				wpa_printf(MSG_DEBUG, "P2P: Cannot force GO on any of the channels we are already using. Use one of the free channels");
			}
		}
	} else {
		for (i = 0; i < num; i++) {
			if (freqs[i].freq == freq)
				break;
		}

		if (i == num) {
			if (wpas_p2p_num_unused_channels(wpa_s) <= 0) {
				if (freq)
					wpa_printf(MSG_DEBUG, "P2P: Cannot force GO on freq (%u MHz) as all the channels are in use", freq);
				os_free(freqs);
				return -1;
			} else {
				wpa_printf(MSG_DEBUG, "P2P: Use one of the free channels");
			}
		}
	}

	os_free(freqs);
	return 0;
}


static struct wpa_supplicant *
wpas_p2p_get_group_iface(struct wpa_supplicant *wpa_s, int addr_allocated,
			 int go)
{
	struct wpa_supplicant *group_wpa_s;

	if (!wpas_p2p_create_iface(wpa_s)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Use same interface for group "
			"operations");
		wpa_s->p2p_first_connection_timeout = 0;
		return wpa_s;
	}

	if (wpas_p2p_add_group_interface(wpa_s, go ? WPA_IF_P2P_GO :
					 WPA_IF_P2P_CLIENT) < 0) {
		wpa_msg_global(wpa_s, MSG_ERROR,
			       "P2P: Failed to add group interface");
		return NULL;
	}
	group_wpa_s = wpas_p2p_init_group_interface(wpa_s, go);
	if (group_wpa_s == NULL) {
		wpa_msg_global(wpa_s, MSG_ERROR,
			       "P2P: Failed to initialize group interface");
		wpas_p2p_remove_pending_group_interface(wpa_s);
		return NULL;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Use separate group interface %s",
		group_wpa_s->ifname);
	group_wpa_s->p2p_first_connection_timeout = 0;
	return group_wpa_s;
}


/**
 * wpas_p2p_group_add - Add a new P2P group with local end as Group Owner
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supplicant_add_iface()
 * @persistent_group: Whether to create a persistent group
 * @freq: Frequency for the group or 0 to indicate no hardcoding
 * @ht40: Start GO with 40 MHz channel width
 * @vht:  Start GO with VHT support
 * Returns: 0 on success, -1 on failure
 *
 * This function creates a new P2P group with the local end as the Group Owner,
 * i.e., without using Group Owner Negotiation.
 */
int wpas_p2p_group_add(struct wpa_supplicant *wpa_s, int persistent_group,
		       int freq, int ht40, int vht)
{
	struct p2p_go_neg_results params;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	/* Make sure we are not running find during connection establishment */
	wpa_printf(MSG_DEBUG, "P2P: Stop any on-going P2P FIND");
	wpas_p2p_stop_find_oper(wpa_s);

	freq = wpas_p2p_select_go_freq(wpa_s, freq);
	if (freq < 0)
		return -1;

	if (wpas_p2p_init_go_params(wpa_s, &params, freq, ht40, vht, NULL))
		return -1;
	if (params.freq &&
	    !p2p_supported_freq_go(wpa_s->global->p2p, params.freq)) {
		wpa_printf(MSG_DEBUG, "P2P: The selected channel for GO "
			   "(%u MHz) is not supported for P2P uses",
			   params.freq);
		return -1;
	}
	p2p_go_params(wpa_s->global->p2p, &params);
	params.persistent_group = persistent_group;

	wpa_s = wpas_p2p_get_group_iface(wpa_s, 0, 1);
	if (wpa_s == NULL)
		return -1;
	wpas_start_wps_go(wpa_s, &params, 0);

	return 0;
}


static int wpas_start_p2p_client(struct wpa_supplicant *wpa_s,
				 struct wpa_ssid *params, int addr_allocated,
				 int freq)
{
	struct wpa_ssid *ssid;

	wpa_s = wpas_p2p_get_group_iface(wpa_s, addr_allocated, 0);
	if (wpa_s == NULL)
		return -1;
	wpa_s->p2p_last_4way_hs_fail = NULL;

	wpa_supplicant_ap_deinit(wpa_s);

	ssid = wpa_config_add_network(wpa_s->conf);
	if (ssid == NULL)
		return -1;
	wpa_config_set_network_defaults(ssid);
	ssid->temporary = 1;
	ssid->proto = WPA_PROTO_RSN;
	ssid->pairwise_cipher = WPA_CIPHER_CCMP;
	ssid->group_cipher = WPA_CIPHER_CCMP;
	ssid->key_mgmt = WPA_KEY_MGMT_PSK;
	ssid->ssid = os_malloc(params->ssid_len);
	if (ssid->ssid == NULL) {
		wpa_config_remove_network(wpa_s->conf, ssid->id);
		return -1;
	}
	os_memcpy(ssid->ssid, params->ssid, params->ssid_len);
	ssid->ssid_len = params->ssid_len;
	ssid->p2p_group = 1;
	ssid->export_keys = 1;
	if (params->psk_set) {
		os_memcpy(ssid->psk, params->psk, 32);
		ssid->psk_set = 1;
	}
	if (params->passphrase)
		ssid->passphrase = os_strdup(params->passphrase);

	wpa_s->show_group_started = 1;
	wpa_s->p2p_in_invitation = 1;
	wpa_s->p2p_invite_go_freq = freq;

	eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s->parent,
			     NULL);
	eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT, 0,
			       wpas_p2p_group_formation_timeout,
			       wpa_s->parent, NULL);
	wpa_supplicant_select_network(wpa_s, ssid);

	return 0;
}


int wpas_p2p_group_add_persistent(struct wpa_supplicant *wpa_s,
				  struct wpa_ssid *ssid, int addr_allocated,
				  int force_freq, int neg_freq, int ht40,
				  int vht, const struct p2p_channels *channels,
				  int connection_timeout)
{
	struct p2p_go_neg_results params;
	int go = 0, freq;

	if (ssid->disabled != 2 || ssid->ssid == NULL)
		return -1;

	if (wpas_get_p2p_group(wpa_s, ssid->ssid, ssid->ssid_len, &go) &&
	    go == (ssid->mode == WPAS_MODE_P2P_GO)) {
		wpa_printf(MSG_DEBUG, "P2P: Requested persistent group is "
			   "already running");
		return 0;
	}

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	/* Make sure we are not running find during connection establishment */
	wpas_p2p_stop_find_oper(wpa_s);

	wpa_s->p2p_fallback_to_go_neg = 0;

	if (force_freq > 0) {
		freq = wpas_p2p_select_go_freq(wpa_s, force_freq);
		if (freq < 0)
			return -1;
	} else {
		freq = wpas_p2p_select_go_freq(wpa_s, neg_freq);
		if (freq < 0 || (freq > 0 && !freq_included(channels, freq)))
			freq = 0;
	}

	if (ssid->mode == WPAS_MODE_INFRA)
		return wpas_start_p2p_client(wpa_s, ssid, addr_allocated, freq);

	if (ssid->mode != WPAS_MODE_P2P_GO)
		return -1;

	if (wpas_p2p_init_go_params(wpa_s, &params, freq, ht40, vht, channels))
		return -1;

	params.role_go = 1;
	params.psk_set = ssid->psk_set;
	if (params.psk_set)
		os_memcpy(params.psk, ssid->psk, sizeof(params.psk));
	if (ssid->passphrase) {
		if (os_strlen(ssid->passphrase) >= sizeof(params.passphrase)) {
			wpa_printf(MSG_ERROR, "P2P: Invalid passphrase in "
				   "persistent group");
			return -1;
		}
		os_strlcpy(params.passphrase, ssid->passphrase,
			   sizeof(params.passphrase));
	}
	os_memcpy(params.ssid, ssid->ssid, ssid->ssid_len);
	params.ssid_len = ssid->ssid_len;
	params.persistent_group = 1;

	wpa_s = wpas_p2p_get_group_iface(wpa_s, addr_allocated, 1);
	if (wpa_s == NULL)
		return -1;

	wpa_s->p2p_first_connection_timeout = connection_timeout;
	wpas_start_wps_go(wpa_s, &params, 0);

	return 0;
}


static void wpas_p2p_ie_update(void *ctx, struct wpabuf *beacon_ies,
			       struct wpabuf *proberesp_ies)
{
	struct wpa_supplicant *wpa_s = ctx;
	if (wpa_s->ap_iface) {
		struct hostapd_data *hapd = wpa_s->ap_iface->bss[0];
		if (!(hapd->conf->p2p & P2P_GROUP_OWNER)) {
			wpabuf_free(beacon_ies);
			wpabuf_free(proberesp_ies);
			return;
		}
		if (beacon_ies) {
			wpabuf_free(hapd->p2p_beacon_ie);
			hapd->p2p_beacon_ie = beacon_ies;
		}
		wpabuf_free(hapd->p2p_probe_resp_ie);
		hapd->p2p_probe_resp_ie = proberesp_ies;
	} else {
		wpabuf_free(beacon_ies);
		wpabuf_free(proberesp_ies);
	}
	wpa_supplicant_ap_update_beacon(wpa_s);
}


static void wpas_p2p_idle_update(void *ctx, int idle)
{
	struct wpa_supplicant *wpa_s = ctx;
	if (!wpa_s->ap_iface)
		return;
	wpa_printf(MSG_DEBUG, "P2P: GO - group %sidle", idle ? "" : "not ");
	if (idle) {
		if (wpa_s->global->p2p_fail_on_wps_complete &&
		    wpa_s->p2p_in_provisioning) {
			wpas_p2p_grpform_fail_after_wps(wpa_s);
			return;
		}
		wpas_p2p_set_group_idle_timeout(wpa_s);
	} else
		eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL);
}


struct p2p_group * wpas_p2p_group_init(struct wpa_supplicant *wpa_s,
				       struct wpa_ssid *ssid)
{
	struct p2p_group *group;
	struct p2p_group_config *cfg;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return NULL;

	cfg = os_zalloc(sizeof(*cfg));
	if (cfg == NULL)
		return NULL;

	if (ssid->p2p_persistent_group && wpa_s->conf->persistent_reconnect)
		cfg->persistent_group = 2;
	else if (ssid->p2p_persistent_group)
		cfg->persistent_group = 1;
	os_memcpy(cfg->interface_addr, wpa_s->own_addr, ETH_ALEN);
	if (wpa_s->max_stations &&
	    wpa_s->max_stations < wpa_s->conf->max_num_sta)
		cfg->max_clients = wpa_s->max_stations;
	else
		cfg->max_clients = wpa_s->conf->max_num_sta;
	os_memcpy(cfg->ssid, ssid->ssid, ssid->ssid_len);
	cfg->ssid_len = ssid->ssid_len;
	cfg->freq = ssid->frequency;
	cfg->cb_ctx = wpa_s;
	cfg->ie_update = wpas_p2p_ie_update;
	cfg->idle_update = wpas_p2p_idle_update;

	group = p2p_group_init(wpa_s->global->p2p, cfg);
	if (group == NULL)
		os_free(cfg);
	if (ssid->mode != WPAS_MODE_P2P_GROUP_FORMATION)
		p2p_group_notif_formation_done(group);
	wpa_s->p2p_group = group;
	return group;
}


void wpas_p2p_wps_success(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
			  int registrar)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (!wpa_s->p2p_in_provisioning) {
		wpa_printf(MSG_DEBUG, "P2P: Ignore WPS success event - P2P "
			   "provisioning not in progress");
		return;
	}

	if (ssid && ssid->mode == WPAS_MODE_INFRA) {
		u8 go_dev_addr[ETH_ALEN];
		os_memcpy(go_dev_addr, wpa_s->bssid, ETH_ALEN);
		wpas_p2p_persistent_group(wpa_s, go_dev_addr, ssid->ssid,
					  ssid->ssid_len);
		/* Clear any stored provisioning info */
		p2p_clear_provisioning_info(wpa_s->global->p2p, go_dev_addr);
	}

	eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s->parent,
			     NULL);
	wpa_s->p2p_go_group_formation_completed = 1;
	if (ssid && ssid->mode == WPAS_MODE_INFRA) {
		/*
		 * Use a separate timeout for initial data connection to
		 * complete to allow the group to be removed automatically if
		 * something goes wrong in this step before the P2P group idle
		 * timeout mechanism is taken into use.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG,
			"P2P: Re-start group formation timeout (%d seconds) as client for initial connection",
			P2P_MAX_INITIAL_CONN_WAIT);
		eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT, 0,
				       wpas_p2p_group_formation_timeout,
				       wpa_s->parent, NULL);
	} else if (ssid) {
		/*
		 * Use a separate timeout for initial data connection to
		 * complete to allow the group to be removed automatically if
		 * the client does not complete data connection successfully.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG,
			"P2P: Re-start group formation timeout (%d seconds) as GO for initial connection",
			P2P_MAX_INITIAL_CONN_WAIT_GO);
		eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT_GO, 0,
				       wpas_p2p_group_formation_timeout,
				       wpa_s->parent, NULL);
		/*
		 * Complete group formation on first successful data connection
		 */
		wpa_s->p2p_go_group_formation_completed = 0;
	}
	if (wpa_s->global->p2p)
		p2p_wps_success_cb(wpa_s->global->p2p, peer_addr);
	wpas_group_formation_completed(wpa_s, 1);
}


void wpas_p2p_wps_failed(struct wpa_supplicant *wpa_s,
			 struct wps_event_fail *fail)
{
	if (!wpa_s->p2p_in_provisioning) {
		wpa_printf(MSG_DEBUG, "P2P: Ignore WPS fail event - P2P "
			   "provisioning not in progress");
		return;
	}

	if (wpa_s->go_params) {
		p2p_clear_provisioning_info(
			wpa_s->global->p2p,
			wpa_s->go_params->peer_device_addr);
	}

	wpas_notify_p2p_wps_failed(wpa_s, fail);

	if (wpa_s == wpa_s->global->p2p_group_formation) {
		/*
		 * Allow some time for the failed WPS negotiation exchange to
		 * complete, but remove the group since group formation cannot
		 * succeed after provisioning failure.
		 */
		wpa_printf(MSG_DEBUG, "P2P: WPS step failed during group formation - reject connection from timeout");
		wpa_s->global->p2p_fail_on_wps_complete = 1;
		eloop_deplete_timeout(0, 50000,
				      wpas_p2p_group_formation_timeout,
				      wpa_s->parent, NULL);
	}
}


int wpas_p2p_wps_eapol_cb(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->global->p2p_fail_on_wps_complete ||
	    !wpa_s->p2p_in_provisioning)
		return 0;

	wpas_p2p_grpform_fail_after_wps(wpa_s);

	return 1;
}


int wpas_p2p_prov_disc(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
		       const char *config_method,
		       enum wpas_p2p_prov_disc_use use)
{
	u16 config_methods;

	wpa_s->p2p_fallback_to_go_neg = 0;
	wpa_s->pending_pd_use = NORMAL_PD;
	if (os_strncmp(config_method, "display", 7) == 0)
		config_methods = WPS_CONFIG_DISPLAY;
	else if (os_strncmp(config_method, "keypad", 6) == 0)
		config_methods = WPS_CONFIG_KEYPAD;
	else if (os_strncmp(config_method, "pbc", 3) == 0 ||
		 os_strncmp(config_method, "pushbutton", 10) == 0)
		config_methods = WPS_CONFIG_PUSHBUTTON;
	else {
		wpa_printf(MSG_DEBUG, "P2P: Unknown config method");
		return -1;
	}

	if (use == WPAS_P2P_PD_AUTO) {
		os_memcpy(wpa_s->pending_join_dev_addr, peer_addr, ETH_ALEN);
		wpa_s->pending_pd_config_methods = config_methods;
		wpa_s->p2p_auto_pd = 1;
		wpa_s->p2p_auto_join = 0;
		wpa_s->pending_pd_before_join = 0;
		wpa_s->auto_pd_scan_retry = 0;
		wpas_p2p_stop_find(wpa_s);
		wpa_s->p2p_join_scan_count = 0;
		os_get_reltime(&wpa_s->p2p_auto_started);
		wpa_printf(MSG_DEBUG, "P2P: Auto PD started at %ld.%06ld",
			   wpa_s->p2p_auto_started.sec,
			   wpa_s->p2p_auto_started.usec);
		wpas_p2p_join_scan(wpa_s, NULL);
		return 0;
	}

	if (wpa_s->global->p2p == NULL || wpa_s->global->p2p_disabled)
		return -1;

	return p2p_prov_disc_req(wpa_s->global->p2p, peer_addr,
				 config_methods, use == WPAS_P2P_PD_FOR_JOIN,
				 0, 1);
}


int wpas_p2p_scan_result_text(const u8 *ies, size_t ies_len, char *buf,
			      char *end)
{
	return p2p_scan_result_text(ies, ies_len, buf, end);
}


static void wpas_p2p_clear_pending_action_tx(struct wpa_supplicant *wpa_s)
{
	if (!offchannel_pending_action_tx(wpa_s))
		return;

	wpas_p2p_action_tx_clear(wpa_s);

	wpa_printf(MSG_DEBUG, "P2P: Drop pending Action TX due to new "
		   "operation request");
	offchannel_clear_pending_action_tx(wpa_s);
}


int wpas_p2p_find(struct wpa_supplicant *wpa_s, unsigned int timeout,
		  enum p2p_discovery_type type,
		  unsigned int num_req_dev_types, const u8 *req_dev_types,
		  const u8 *dev_id, unsigned int search_delay)
{
	wpas_p2p_clear_pending_action_tx(wpa_s);
	wpa_s->p2p_long_listen = 0;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL ||
	    wpa_s->p2p_in_provisioning)
		return -1;

	wpa_supplicant_cancel_sched_scan(wpa_s);

	return p2p_find(wpa_s->global->p2p, timeout, type,
			num_req_dev_types, req_dev_types, dev_id,
			search_delay);
}


static int wpas_p2p_stop_find_oper(struct wpa_supplicant *wpa_s)
{
	wpas_p2p_clear_pending_action_tx(wpa_s);
	wpa_s->p2p_long_listen = 0;
	eloop_cancel_timeout(wpas_p2p_long_listen_timeout, wpa_s, NULL);
	eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);

	if (wpa_s->global->p2p)
		p2p_stop_find(wpa_s->global->p2p);

	return 0;
}


void wpas_p2p_stop_find(struct wpa_supplicant *wpa_s)
{
	if (wpas_p2p_stop_find_oper(wpa_s) > 0)
		return;
	wpas_p2p_remove_pending_group_interface(wpa_s);
}


static void wpas_p2p_long_listen_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpa_s->p2p_long_listen = 0;
}


int wpas_p2p_listen(struct wpa_supplicant *wpa_s, unsigned int timeout)
{
	int res;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	wpa_supplicant_cancel_sched_scan(wpa_s);
	wpas_p2p_clear_pending_action_tx(wpa_s);

	if (timeout == 0) {
		/*
		 * This is a request for unlimited Listen state. However, at
		 * least for now, this is mapped to a Listen state for one
		 * hour.
		 */
		timeout = 3600;
	}
	eloop_cancel_timeout(wpas_p2p_long_listen_timeout, wpa_s, NULL);
	wpa_s->p2p_long_listen = 0;

	/*
	 * Stop previous find/listen operation to avoid trying to request a new
	 * remain-on-channel operation while the driver is still running the
	 * previous one.
	 */
	if (wpa_s->global->p2p)
		p2p_stop_find(wpa_s->global->p2p);

	res = wpas_p2p_listen_start(wpa_s, timeout * 1000);
	if (res == 0 && timeout * 1000 > wpa_s->max_remain_on_chan) {
		wpa_s->p2p_long_listen = timeout * 1000;
		eloop_register_timeout(timeout, 0,
				       wpas_p2p_long_listen_timeout,
				       wpa_s, NULL);
	}

	return res;
}


int wpas_p2p_assoc_req_ie(struct wpa_supplicant *wpa_s, struct wpa_bss *bss,
			  u8 *buf, size_t len, int p2p_group)
{
	struct wpabuf *p2p_ie;
	int ret;

	if (wpa_s->global->p2p_disabled)
		return -1;
	if (wpa_s->conf->p2p_disabled)
		return -1;
	if (wpa_s->global->p2p == NULL)
		return -1;
	if (bss == NULL)
		return -1;

	p2p_ie = wpa_bss_get_vendor_ie_multi(bss, P2P_IE_VENDOR_TYPE);
	ret = p2p_assoc_req_ie(wpa_s->global->p2p, bss->bssid, buf, len,
			       p2p_group, p2p_ie);
	wpabuf_free(p2p_ie);

	return ret;
}


int wpas_p2p_probe_req_rx(struct wpa_supplicant *wpa_s, const u8 *addr,
			  const u8 *dst, const u8 *bssid,
			  const u8 *ie, size_t ie_len, int ssi_signal)
{
	if (wpa_s->global->p2p_disabled)
		return 0;
	if (wpa_s->global->p2p == NULL)
		return 0;

	switch (p2p_probe_req_rx(wpa_s->global->p2p, addr, dst, bssid,
				 ie, ie_len)) {
	case P2P_PREQ_NOT_P2P:
		wpas_notify_preq(wpa_s, addr, dst, bssid, ie, ie_len,
				 ssi_signal);
		/* fall through */
	case P2P_PREQ_MALFORMED:
	case P2P_PREQ_NOT_LISTEN:
	case P2P_PREQ_NOT_PROCESSED:
	default: /* make gcc happy */
		return 0;
	case P2P_PREQ_PROCESSED:
		return 1;
	}
}


void wpas_p2p_rx_action(struct wpa_supplicant *wpa_s, const u8 *da,
			const u8 *sa, const u8 *bssid,
			u8 category, const u8 *data, size_t len, int freq)
{
	if (wpa_s->global->p2p_disabled)
		return;
	if (wpa_s->global->p2p == NULL)
		return;

	p2p_rx_action(wpa_s->global->p2p, da, sa, bssid, category, data, len,
		      freq);
}


void wpas_p2p_scan_ie(struct wpa_supplicant *wpa_s, struct wpabuf *ies)
{
	if (wpa_s->global->p2p_disabled)
		return;
	if (wpa_s->global->p2p == NULL)
		return;

	p2p_scan_ie(wpa_s->global->p2p, ies, NULL);
}


static void wpas_p2p_group_deinit(struct wpa_supplicant *wpa_s)
{
	p2p_group_deinit(wpa_s->p2p_group);
	wpa_s->p2p_group = NULL;

	wpa_s->ap_configured_cb = NULL;
	wpa_s->ap_configured_cb_ctx = NULL;
	wpa_s->ap_configured_cb_data = NULL;
	wpa_s->connect_without_scan = NULL;
}


int wpas_p2p_reject(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	wpa_s->p2p_long_listen = 0;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_reject(wpa_s->global->p2p, addr);
}


/* Invite to reinvoke a persistent group */
int wpas_p2p_invite(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
		    struct wpa_ssid *ssid, const u8 *go_dev_addr, int freq,
		    int ht40, int vht, int pref_freq)
{
	enum p2p_invite_role role;
	u8 *bssid = NULL;
	int force_freq = 0;
	int res;
	int no_pref_freq_given = pref_freq == 0;

	wpa_s->global->p2p_invite_group = NULL;
	if (peer_addr)
		os_memcpy(wpa_s->p2p_auth_invite, peer_addr, ETH_ALEN);
	else
		os_memset(wpa_s->p2p_auth_invite, 0, ETH_ALEN);

	wpa_s->p2p_persistent_go_freq = freq;
	wpa_s->p2p_go_ht40 = !!ht40;
	if (ssid->mode == WPAS_MODE_P2P_GO) {
		role = P2P_INVITE_ROLE_GO;
		if (peer_addr == NULL) {
			wpa_printf(MSG_DEBUG, "P2P: Missing peer "
				   "address in invitation command");
			return -1;
		}
		if (wpas_p2p_create_iface(wpa_s)) {
			if (wpas_p2p_add_group_interface(wpa_s,
							 WPA_IF_P2P_GO) < 0) {
				wpa_printf(MSG_ERROR, "P2P: Failed to "
					   "allocate a new interface for the "
					   "group");
				return -1;
			}
			bssid = wpa_s->pending_interface_addr;
		} else
			bssid = wpa_s->own_addr;
	} else {
		role = P2P_INVITE_ROLE_CLIENT;
		peer_addr = ssid->bssid;
	}
	wpa_s->pending_invite_ssid_id = ssid->id;

	res = wpas_p2p_setup_freqs(wpa_s, freq, &force_freq, &pref_freq,
				   role == P2P_INVITE_ROLE_GO);
	if (res)
		return res;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (wpa_s->parent->conf->p2p_ignore_shared_freq &&
	    no_pref_freq_given && pref_freq > 0 &&
	    wpa_s->num_multichan_concurrent > 1 &&
	    wpas_p2p_num_unused_channels(wpa_s) > 0) {
		wpa_printf(MSG_DEBUG, "P2P: Ignore own channel preference %d MHz for invitation due to p2p_ignore_shared_freq=1 configuration",
			   pref_freq);
		pref_freq = 0;
	}

	return p2p_invite(wpa_s->global->p2p, peer_addr, role, bssid,
			  ssid->ssid, ssid->ssid_len, force_freq, go_dev_addr,
			  1, pref_freq, -1);
}


/* Invite to join an active group */
int wpas_p2p_invite_group(struct wpa_supplicant *wpa_s, const char *ifname,
			  const u8 *peer_addr, const u8 *go_dev_addr)
{
	struct wpa_global *global = wpa_s->global;
	enum p2p_invite_role role;
	u8 *bssid = NULL;
	struct wpa_ssid *ssid;
	int persistent;
	int freq = 0, force_freq = 0, pref_freq = 0;
	int res;

	wpa_s->p2p_persistent_go_freq = 0;
	wpa_s->p2p_go_ht40 = 0;
	wpa_s->p2p_go_vht = 0;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp(wpa_s->ifname, ifname) == 0)
			break;
	}
	if (wpa_s == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Interface '%s' not found", ifname);
		return -1;
	}

	ssid = wpa_s->current_ssid;
	if (ssid == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: No current SSID to use for "
			   "invitation");
		return -1;
	}

	wpa_s->global->p2p_invite_group = wpa_s;
	persistent = ssid->p2p_persistent_group &&
		wpas_p2p_get_persistent(wpa_s->parent, peer_addr,
					ssid->ssid, ssid->ssid_len);

	if (ssid->mode == WPAS_MODE_P2P_GO) {
		role = P2P_INVITE_ROLE_ACTIVE_GO;
		bssid = wpa_s->own_addr;
		if (go_dev_addr == NULL)
			go_dev_addr = wpa_s->global->p2p_dev_addr;
		freq = ssid->frequency;
	} else {
		role = P2P_INVITE_ROLE_CLIENT;
		if (wpa_s->wpa_state < WPA_ASSOCIATED) {
			wpa_printf(MSG_DEBUG, "P2P: Not associated - cannot "
				   "invite to current group");
			return -1;
		}
		bssid = wpa_s->bssid;
		if (go_dev_addr == NULL &&
		    !is_zero_ether_addr(wpa_s->go_dev_addr))
			go_dev_addr = wpa_s->go_dev_addr;
		freq = wpa_s->current_bss ? wpa_s->current_bss->freq :
			(int) wpa_s->assoc_freq;
	}
	wpa_s->parent->pending_invite_ssid_id = -1;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	res = wpas_p2p_setup_freqs(wpa_s, freq, &force_freq, &pref_freq,
				   role == P2P_INVITE_ROLE_ACTIVE_GO);
	if (res)
		return res;
	wpas_p2p_set_own_freq_preference(wpa_s, force_freq);

	return p2p_invite(wpa_s->global->p2p, peer_addr, role, bssid,
			  ssid->ssid, ssid->ssid_len, force_freq,
			  go_dev_addr, persistent, pref_freq, -1);
}


void wpas_p2p_completed(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	u8 go_dev_addr[ETH_ALEN];
	int network_id = -1;
	int persistent;
	int freq;
	u8 ip[3 * 4];
	char ip_addr[100];

	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GROUP_FORMATION) {
		eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				     wpa_s->parent, NULL);
	}

	if (!wpa_s->show_group_started || !ssid)
		return;

	wpa_s->show_group_started = 0;

	os_memset(go_dev_addr, 0, ETH_ALEN);
	if (ssid->bssid_set)
		os_memcpy(go_dev_addr, ssid->bssid, ETH_ALEN);
	persistent = wpas_p2p_persistent_group(wpa_s, go_dev_addr, ssid->ssid,
					       ssid->ssid_len);
	os_memcpy(wpa_s->go_dev_addr, go_dev_addr, ETH_ALEN);

	if (wpa_s->global->p2p_group_formation == wpa_s)
		wpa_s->global->p2p_group_formation = NULL;

	freq = wpa_s->current_bss ? wpa_s->current_bss->freq :
		(int) wpa_s->assoc_freq;

	ip_addr[0] = '\0';
	if (wpa_sm_get_p2p_ip_addr(wpa_s->wpa, ip) == 0) {
		os_snprintf(ip_addr, sizeof(ip_addr), " ip_addr=%u.%u.%u.%u "
			    "ip_mask=%u.%u.%u.%u go_ip_addr=%u.%u.%u.%u",
			    ip[0], ip[1], ip[2], ip[3],
			    ip[4], ip[5], ip[6], ip[7],
			    ip[8], ip[9], ip[10], ip[11]);
	}

	wpas_p2p_group_started(wpa_s, 0, ssid, freq,
			       ssid->passphrase == NULL && ssid->psk_set ?
			       ssid->psk : NULL,
			       ssid->passphrase, go_dev_addr, persistent,
			       ip_addr);

	if (persistent)
		network_id = wpas_p2p_store_persistent_group(wpa_s->parent,
							     ssid, go_dev_addr);
	if (network_id < 0)
		network_id = ssid->id;
	wpas_notify_p2p_group_started(wpa_s, ssid, network_id, 1);
}


int wpas_p2p_presence_req(struct wpa_supplicant *wpa_s, u32 duration1,
			  u32 interval1, u32 duration2, u32 interval2)
{
	int ret;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (wpa_s->wpa_state < WPA_ASSOCIATED ||
	    wpa_s->current_ssid == NULL ||
	    wpa_s->current_ssid->mode != WPAS_MODE_INFRA)
		return -1;

	ret = p2p_presence_req(wpa_s->global->p2p, wpa_s->bssid,
			       wpa_s->own_addr, wpa_s->assoc_freq,
			       duration1, interval1, duration2, interval2);
	if (ret == 0)
		wpa_s->waiting_presence_resp = 1;

	return ret;
}


int wpas_p2p_ext_listen(struct wpa_supplicant *wpa_s, unsigned int period,
			unsigned int interval)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_ext_listen(wpa_s->global->p2p, period, interval);
}


static int wpas_p2p_is_client(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL) {
		/*
		 * current_ssid can be cleared when P2P client interface gets
		 * disconnected, so assume this interface was used as P2P
		 * client.
		 */
		return 1;
	}
	return wpa_s->current_ssid->p2p_group &&
		wpa_s->current_ssid->mode == WPAS_MODE_INFRA;
}


static void wpas_p2p_group_idle_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (wpa_s->conf->p2p_group_idle == 0 && !wpas_p2p_is_client(wpa_s)) {
		wpa_printf(MSG_DEBUG, "P2P: Ignore group idle timeout - "
			   "disabled");
		return;
	}

	wpa_printf(MSG_DEBUG, "P2P: Group idle timeout reached - terminate "
		   "group");
	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_IDLE_TIMEOUT);
}


static void wpas_p2p_set_group_idle_timeout(struct wpa_supplicant *wpa_s)
{
	int timeout;

	if (eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL) > 0)
		wpa_printf(MSG_DEBUG, "P2P: Cancelled P2P group idle timeout");

	if (wpa_s->current_ssid == NULL || !wpa_s->current_ssid->p2p_group)
		return;

	timeout = wpa_s->conf->p2p_group_idle;
	if (wpa_s->current_ssid->mode == WPAS_MODE_INFRA &&
	    (timeout == 0 || timeout > P2P_MAX_CLIENT_IDLE))
	    timeout = P2P_MAX_CLIENT_IDLE;

	if (timeout == 0)
		return;

	if (timeout < 0) {
		if (wpa_s->current_ssid->mode == WPAS_MODE_INFRA)
			timeout = 0; /* special client mode no-timeout */
		else
			return;
	}

	if (wpa_s->p2p_in_provisioning) {
		/*
		 * Use the normal group formation timeout during the
		 * provisioning phase to avoid terminating this process too
		 * early due to group idle timeout.
		 */
		wpa_printf(MSG_DEBUG, "P2P: Do not use P2P group idle timeout "
			   "during provisioning");
		return;
	}

	if (wpa_s->show_group_started) {
		/*
		 * Use the normal group formation timeout between the end of
		 * the provisioning phase and completion of 4-way handshake to
		 * avoid terminating this process too early due to group idle
		 * timeout.
		 */
		wpa_printf(MSG_DEBUG, "P2P: Do not use P2P group idle timeout "
			   "while waiting for initial 4-way handshake to "
			   "complete");
		return;
	}

	wpa_printf(MSG_DEBUG, "P2P: Set P2P group idle timeout to %u seconds",
		   timeout);
	eloop_register_timeout(timeout, 0, wpas_p2p_group_idle_timeout,
			       wpa_s, NULL);
}


/* Returns 1 if the interface was removed */
int wpas_p2p_deauth_notif(struct wpa_supplicant *wpa_s, const u8 *bssid,
			  u16 reason_code, const u8 *ie, size_t ie_len,
			  int locally_generated)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;

	if (!locally_generated)
		p2p_deauth_notif(wpa_s->global->p2p, bssid, reason_code, ie,
				 ie_len);

	if (reason_code == WLAN_REASON_DEAUTH_LEAVING && !locally_generated &&
	    wpa_s->current_ssid &&
	    wpa_s->current_ssid->p2p_group &&
	    wpa_s->current_ssid->mode == WPAS_MODE_INFRA) {
		wpa_printf(MSG_DEBUG, "P2P: GO indicated that the P2P Group "
			   "session is ending");
		if (wpas_p2p_group_delete(wpa_s,
					  P2P_GROUP_REMOVAL_GO_ENDING_SESSION)
		    > 0)
			return 1;
	}

	return 0;
}


void wpas_p2p_disassoc_notif(struct wpa_supplicant *wpa_s, const u8 *bssid,
			     u16 reason_code, const u8 *ie, size_t ie_len,
			     int locally_generated)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;

	if (!locally_generated)
		p2p_disassoc_notif(wpa_s->global->p2p, bssid, reason_code, ie,
				   ie_len);
}


void wpas_p2p_update_config(struct wpa_supplicant *wpa_s)
{
	struct p2p_data *p2p = wpa_s->global->p2p;

	if (p2p == NULL)
		return;

	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CAPABLE))
		return;

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_DEVICE_NAME)
		p2p_set_dev_name(p2p, wpa_s->conf->device_name);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_DEVICE_TYPE)
		p2p_set_pri_dev_type(p2p, wpa_s->conf->device_type);

	if (wpa_s->wps &&
	    (wpa_s->conf->changed_parameters & CFG_CHANGED_CONFIG_METHODS))
		p2p_set_config_methods(p2p, wpa_s->wps->config_methods);

	if (wpa_s->wps && (wpa_s->conf->changed_parameters & CFG_CHANGED_UUID))
		p2p_set_uuid(p2p, wpa_s->wps->uuid);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_WPS_STRING) {
		p2p_set_manufacturer(p2p, wpa_s->conf->manufacturer);
		p2p_set_model_name(p2p, wpa_s->conf->model_name);
		p2p_set_model_number(p2p, wpa_s->conf->model_number);
		p2p_set_serial_number(p2p, wpa_s->conf->serial_number);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_SEC_DEVICE_TYPE)
		p2p_set_sec_dev_types(p2p,
				      (void *) wpa_s->conf->sec_device_type,
				      wpa_s->conf->num_sec_device_types);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_VENDOR_EXTENSION) {
		int i;
		p2p_remove_wps_vendor_extensions(p2p);
		for (i = 0; i < MAX_WPS_VENDOR_EXT; i++) {
			if (wpa_s->conf->wps_vendor_ext[i] == NULL)
				continue;
			p2p_add_wps_vendor_extension(
				p2p, wpa_s->conf->wps_vendor_ext[i]);
		}
	}

	if ((wpa_s->conf->changed_parameters & CFG_CHANGED_COUNTRY) &&
	    wpa_s->conf->country[0] && wpa_s->conf->country[1]) {
		char country[3];
		country[0] = wpa_s->conf->country[0];
		country[1] = wpa_s->conf->country[1];
		country[2] = 0x04;
		p2p_set_country(p2p, country);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_SSID_POSTFIX) {
		p2p_set_ssid_postfix(p2p, (u8 *) wpa_s->conf->p2p_ssid_postfix,
				     wpa_s->conf->p2p_ssid_postfix ?
				     os_strlen(wpa_s->conf->p2p_ssid_postfix) :
				     0);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_INTRA_BSS)
		p2p_set_intra_bss_dist(p2p, wpa_s->conf->p2p_intra_bss);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_LISTEN_CHANNEL) {
		u8 reg_class, channel;
		int ret;
		unsigned int r;
		u8 channel_forced;

		if (wpa_s->conf->p2p_listen_reg_class &&
		    wpa_s->conf->p2p_listen_channel) {
			reg_class = wpa_s->conf->p2p_listen_reg_class;
			channel = wpa_s->conf->p2p_listen_channel;
			channel_forced = 1;
		} else {
			reg_class = 81;
			/*
			 * Pick one of the social channels randomly as the
			 * listen channel.
			 */
			if (os_get_random((u8 *) &r, sizeof(r)) < 0)
				channel = 1;
			else
				channel = 1 + (r % 3) * 5;
			channel_forced = 0;
		}
		ret = p2p_set_listen_channel(p2p, reg_class, channel,
					     channel_forced);
		if (ret)
			wpa_printf(MSG_ERROR, "P2P: Own listen channel update "
				   "failed: %d", ret);
	}
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_OPER_CHANNEL) {
		u8 op_reg_class, op_channel, cfg_op_channel;
		int ret = 0;
		unsigned int r;
		if (wpa_s->conf->p2p_oper_reg_class &&
		    wpa_s->conf->p2p_oper_channel) {
			op_reg_class = wpa_s->conf->p2p_oper_reg_class;
			op_channel = wpa_s->conf->p2p_oper_channel;
			cfg_op_channel = 1;
		} else {
			op_reg_class = 81;
			/*
			 * Use random operation channel from (1, 6, 11)
			 *if no other preference is indicated.
			 */
			if (os_get_random((u8 *) &r, sizeof(r)) < 0)
				op_channel = 1;
			else
				op_channel = 1 + (r % 3) * 5;
			cfg_op_channel = 0;
		}
		ret = p2p_set_oper_channel(p2p, op_reg_class, op_channel,
					   cfg_op_channel);
		if (ret)
			wpa_printf(MSG_ERROR, "P2P: Own oper channel update "
				   "failed: %d", ret);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_PREF_CHAN) {
		if (p2p_set_pref_chan(p2p, wpa_s->conf->num_p2p_pref_chan,
				      wpa_s->conf->p2p_pref_chan) < 0) {
			wpa_printf(MSG_ERROR, "P2P: Preferred channel list "
				   "update failed");
		}

		if (p2p_set_no_go_freq(p2p, &wpa_s->conf->p2p_no_go_freq) < 0) {
			wpa_printf(MSG_ERROR, "P2P: No GO channel list "
				   "update failed");
		}
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_PASSPHRASE_LEN)
		p2p_set_passphrase_len(p2p, wpa_s->conf->p2p_passphrase_len);
}


int wpas_p2p_set_noa(struct wpa_supplicant *wpa_s, u8 count, int start,
		     int duration)
{
	if (!wpa_s->ap_iface)
		return -1;
	return hostapd_p2p_set_noa(wpa_s->ap_iface->bss[0], count, start,
				   duration);
}


int wpas_p2p_set_cross_connect(struct wpa_supplicant *wpa_s, int enabled)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	wpa_s->global->cross_connection = enabled;
	p2p_set_cross_connect(wpa_s->global->p2p, enabled);

	if (!enabled) {
		struct wpa_supplicant *iface;

		for (iface = wpa_s->global->ifaces; iface; iface = iface->next)
		{
			if (iface->cross_connect_enabled == 0)
				continue;

			iface->cross_connect_enabled = 0;
			iface->cross_connect_in_use = 0;
			wpa_msg_global(iface->parent, MSG_INFO,
				       P2P_EVENT_CROSS_CONNECT_DISABLE "%s %s",
				       iface->ifname,
				       iface->cross_connect_uplink);
		}
	}

	return 0;
}


static void wpas_p2p_enable_cross_connect(struct wpa_supplicant *uplink)
{
	struct wpa_supplicant *iface;

	if (!uplink->global->cross_connection)
		return;

	for (iface = uplink->global->ifaces; iface; iface = iface->next) {
		if (!iface->cross_connect_enabled)
			continue;
		if (os_strcmp(uplink->ifname, iface->cross_connect_uplink) !=
		    0)
			continue;
		if (iface->ap_iface == NULL)
			continue;
		if (iface->cross_connect_in_use)
			continue;

		iface->cross_connect_in_use = 1;
		wpa_msg_global(iface->parent, MSG_INFO,
			       P2P_EVENT_CROSS_CONNECT_ENABLE "%s %s",
			       iface->ifname, iface->cross_connect_uplink);
	}
}


static void wpas_p2p_disable_cross_connect(struct wpa_supplicant *uplink)
{
	struct wpa_supplicant *iface;

	for (iface = uplink->global->ifaces; iface; iface = iface->next) {
		if (!iface->cross_connect_enabled)
			continue;
		if (os_strcmp(uplink->ifname, iface->cross_connect_uplink) !=
		    0)
			continue;
		if (!iface->cross_connect_in_use)
			continue;

		wpa_msg_global(iface->parent, MSG_INFO,
			       P2P_EVENT_CROSS_CONNECT_DISABLE "%s %s",
			       iface->ifname, iface->cross_connect_uplink);
		iface->cross_connect_in_use = 0;
	}
}


void wpas_p2p_notif_connected(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->ap_iface || wpa_s->current_ssid == NULL ||
	    wpa_s->current_ssid->mode != WPAS_MODE_INFRA ||
	    wpa_s->cross_connect_disallowed)
		wpas_p2p_disable_cross_connect(wpa_s);
	else
		wpas_p2p_enable_cross_connect(wpa_s);
	if (!wpa_s->ap_iface &&
	    eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL) > 0)
		wpa_printf(MSG_DEBUG, "P2P: Cancelled P2P group idle timeout");
}


void wpas_p2p_notif_disconnected(struct wpa_supplicant *wpa_s)
{
	wpas_p2p_disable_cross_connect(wpa_s);
	if (!wpa_s->ap_iface &&
	    !eloop_is_timeout_registered(wpas_p2p_group_idle_timeout,
					 wpa_s, NULL))
		wpas_p2p_set_group_idle_timeout(wpa_s);
}


static void wpas_p2p_cross_connect_setup(struct wpa_supplicant *wpa_s)
{
	struct wpa_supplicant *iface;

	if (!wpa_s->global->cross_connection)
		return;

	for (iface = wpa_s->global->ifaces; iface; iface = iface->next) {
		if (iface == wpa_s)
			continue;
		if (iface->drv_flags &
		    WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE)
			continue;
		if (iface->drv_flags & WPA_DRIVER_FLAGS_P2P_CAPABLE)
			continue;

		wpa_s->cross_connect_enabled = 1;
		os_strlcpy(wpa_s->cross_connect_uplink, iface->ifname,
			   sizeof(wpa_s->cross_connect_uplink));
		wpa_printf(MSG_DEBUG, "P2P: Enable cross connection from "
			   "%s to %s whenever uplink is available",
			   wpa_s->ifname, wpa_s->cross_connect_uplink);

		if (iface->ap_iface || iface->current_ssid == NULL ||
		    iface->current_ssid->mode != WPAS_MODE_INFRA ||
		    iface->cross_connect_disallowed ||
		    iface->wpa_state != WPA_COMPLETED)
			break;

		wpa_s->cross_connect_in_use = 1;
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_CROSS_CONNECT_ENABLE "%s %s",
			       wpa_s->ifname, wpa_s->cross_connect_uplink);
		break;
	}
}


int wpas_p2p_notif_pbc_overlap(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->p2p_group_interface != P2P_GROUP_INTERFACE_CLIENT &&
	    !wpa_s->p2p_in_provisioning)
		return 0; /* not P2P client operation */

	wpa_printf(MSG_DEBUG, "P2P: Terminate connection due to WPS PBC "
		   "session overlap");
	if (wpa_s != wpa_s->parent)
		wpa_msg_ctrl(wpa_s->parent, MSG_INFO, WPS_EVENT_OVERLAP);
	wpas_p2p_group_formation_failed(wpa_s);
	return 1;
}


void wpas_p2p_pbc_overlap_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_notif_pbc_overlap(wpa_s);
}


void wpas_p2p_update_channel_list(struct wpa_supplicant *wpa_s)
{
	struct p2p_channels chan, cli_chan;
	struct wpa_supplicant *ifs;

	if (wpa_s->global == NULL || wpa_s->global->p2p == NULL)
		return;

	os_memset(&chan, 0, sizeof(chan));
	os_memset(&cli_chan, 0, sizeof(cli_chan));
	if (wpas_p2p_setup_channels(wpa_s, &chan, &cli_chan)) {
		wpa_printf(MSG_ERROR, "P2P: Failed to update supported "
			   "channel list");
		return;
	}

	p2p_update_channel_list(wpa_s->global->p2p, &chan, &cli_chan);

	for (ifs = wpa_s->global->ifaces; ifs; ifs = ifs->next) {
		int freq;
		if (!ifs->current_ssid ||
		    !ifs->current_ssid->p2p_group ||
		    (ifs->current_ssid->mode != WPAS_MODE_P2P_GO &&
		     ifs->current_ssid->mode != WPAS_MODE_P2P_GROUP_FORMATION))
				continue;
		freq = ifs->current_ssid->frequency;
		if (freq_included(&chan, freq)) {
			wpa_dbg(ifs, MSG_DEBUG,
				"P2P GO operating frequency %d MHz in valid range",
				freq);
			continue;
		}

		wpa_dbg(ifs, MSG_DEBUG,
			"P2P GO operating in invalid frequency %d MHz",	freq);
		/* TODO: Consider using CSA or removing the group within
		 * wpa_supplicant */
		wpa_msg(ifs, MSG_INFO, P2P_EVENT_REMOVE_AND_REFORM_GROUP);
	}
}


static void wpas_p2p_scan_res_ignore(struct wpa_supplicant *wpa_s,
				     struct wpa_scan_results *scan_res)
{
	wpa_printf(MSG_DEBUG, "P2P: Ignore scan results");
}


int wpas_p2p_cancel(struct wpa_supplicant *wpa_s)
{
	struct wpa_global *global = wpa_s->global;
	int found = 0;
	const u8 *peer;

	if (global->p2p == NULL)
		return -1;

	wpa_printf(MSG_DEBUG, "P2P: Request to cancel group formation");

	if (wpa_s->pending_interface_name[0] &&
	    !is_zero_ether_addr(wpa_s->pending_interface_addr))
		found = 1;

	peer = p2p_get_go_neg_peer(global->p2p);
	if (peer) {
		wpa_printf(MSG_DEBUG, "P2P: Unauthorize pending GO Neg peer "
			   MACSTR, MAC2STR(peer));
		p2p_unauthorize(global->p2p, peer);
		found = 1;
	}

	if (wpa_s->scan_res_handler == wpas_p2p_scan_res_join) {
		wpa_printf(MSG_DEBUG, "P2P: Stop pending scan for join");
		wpa_s->scan_res_handler = wpas_p2p_scan_res_ignore;
		found = 1;
	}

	if (wpa_s->pending_pd_before_join) {
		wpa_printf(MSG_DEBUG, "P2P: Stop pending PD before join");
		wpa_s->pending_pd_before_join = 0;
		found = 1;
	}

	wpas_p2p_stop_find(wpa_s);

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s == global->p2p_group_formation &&
		    (wpa_s->p2p_in_provisioning ||
		     wpa_s->parent->pending_interface_type ==
		     WPA_IF_P2P_CLIENT)) {
			wpa_printf(MSG_DEBUG, "P2P: Interface %s in group "
				   "formation found - cancelling",
				   wpa_s->ifname);
			found = 1;
			eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
					     wpa_s->parent, NULL);
			if (wpa_s->p2p_in_provisioning) {
				wpas_group_formation_completed(wpa_s, 0);
				break;
			}
			wpas_p2p_group_delete(wpa_s,
					      P2P_GROUP_REMOVAL_REQUESTED);
			break;
		} else if (wpa_s->p2p_in_invitation) {
			wpa_printf(MSG_DEBUG, "P2P: Interface %s in invitation found - cancelling",
				   wpa_s->ifname);
			found = 1;
			wpas_p2p_group_formation_failed(wpa_s);
		}
	}

	if (!found) {
		wpa_printf(MSG_DEBUG, "P2P: No ongoing group formation found");
		return -1;
	}

	return 0;
}


void wpas_p2p_interface_unavailable(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL || !wpa_s->current_ssid->p2p_group)
		return;

	wpa_printf(MSG_DEBUG, "P2P: Remove group due to driver resource not "
		   "being available anymore");
	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_UNAVAILABLE);
}


void wpas_p2p_update_best_channels(struct wpa_supplicant *wpa_s,
				   int freq_24, int freq_5, int freq_overall)
{
	struct p2p_data *p2p = wpa_s->global->p2p;
	if (p2p == NULL)
		return;
	p2p_set_best_channels(p2p, freq_24, freq_5, freq_overall);
}


int wpas_p2p_unauthorize(struct wpa_supplicant *wpa_s, const char *addr)
{
	u8 peer[ETH_ALEN];
	struct p2p_data *p2p = wpa_s->global->p2p;

	if (p2p == NULL)
		return -1;

	if (hwaddr_aton(addr, peer))
		return -1;

	return p2p_unauthorize(p2p, peer);
}


/**
 * wpas_p2p_disconnect - Disconnect from a P2P Group
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 on success, -1 on failure
 *
 * This can be used to disconnect from a group in which the local end is a P2P
 * Client or to end a P2P Group in case the local end is the Group Owner. If a
 * virtual network interface was created for this group, that interface will be
 * removed. Otherwise, only the configured P2P group network will be removed
 * from the interface.
 */
int wpas_p2p_disconnect(struct wpa_supplicant *wpa_s)
{

	if (wpa_s == NULL)
		return -1;

	return wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_REQUESTED) < 0 ?
		-1 : 0;
}


int wpas_p2p_in_progress(struct wpa_supplicant *wpa_s)
{
	int ret;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;

	ret = p2p_in_progress(wpa_s->global->p2p);
	if (ret == 0) {
		/*
		 * Check whether there is an ongoing WPS provisioning step (or
		 * other parts of group formation) on another interface since
		 * p2p_in_progress() does not report this to avoid issues for
		 * scans during such provisioning step.
		 */
		if (wpa_s->global->p2p_group_formation &&
		    wpa_s->global->p2p_group_formation != wpa_s) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Another interface (%s) "
				"in group formation",
				wpa_s->global->p2p_group_formation->ifname);
			ret = 1;
		}
	}

	if (!ret && wpa_s->global->p2p_go_wait_client.sec) {
		struct os_reltime now;
		os_get_reltime(&now);
		if (os_reltime_expired(&now, &wpa_s->global->p2p_go_wait_client,
				       P2P_MAX_INITIAL_CONN_WAIT_GO)) {
			/* Wait for the first client has expired */
			wpa_s->global->p2p_go_wait_client.sec = 0;
		} else {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Waiting for initial client connection during group formation");
			ret = 1;
		}
	}

	return ret;
}


void wpas_p2p_network_removed(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid)
{
	if (wpa_s->p2p_in_provisioning && ssid->p2p_group &&
	    eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				 wpa_s->parent, NULL) > 0) {
		/**
		 * Remove the network by scheduling the group formation
		 * timeout to happen immediately. The teardown code
		 * needs to be scheduled to run asynch later so that we
		 * don't delete data from under ourselves unexpectedly.
		 * Calling wpas_p2p_group_formation_timeout directly
		 * causes a series of crashes in WPS failure scenarios.
		 */
		wpa_printf(MSG_DEBUG, "P2P: Canceled group formation due to "
			   "P2P group network getting removed");
		eloop_register_timeout(0, 0, wpas_p2p_group_formation_timeout,
				       wpa_s->parent, NULL);
	}
}


struct wpa_ssid * wpas_p2p_get_persistent(struct wpa_supplicant *wpa_s,
					  const u8 *addr, const u8 *ssid,
					  size_t ssid_len)
{
	struct wpa_ssid *s;
	size_t i;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled != 2)
			continue;
		if (ssid &&
		    (ssid_len != s->ssid_len ||
		     os_memcmp(ssid, s->ssid, ssid_len) != 0))
			continue;
		if (addr == NULL) {
			if (s->mode == WPAS_MODE_P2P_GO)
				return s;
			continue;
		}
		if (os_memcmp(s->bssid, addr, ETH_ALEN) == 0)
			return s; /* peer is GO in the persistent group */
		if (s->mode != WPAS_MODE_P2P_GO || s->p2p_client_list == NULL)
			continue;
		for (i = 0; i < s->num_p2p_clients; i++) {
			if (os_memcmp(s->p2p_client_list + i * ETH_ALEN,
				      addr, ETH_ALEN) == 0)
				return s; /* peer is P2P client in persistent
					   * group */
		}
	}

	return NULL;
}


void wpas_p2p_notify_ap_sta_authorized(struct wpa_supplicant *wpa_s,
				       const u8 *addr)
{
	if (eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				 wpa_s->parent, NULL) > 0) {
		/*
		 * This can happen if WPS provisioning step is not terminated
		 * cleanly (e.g., P2P Client does not send WSC_Done). Since the
		 * peer was able to connect, there is no need to time out group
		 * formation after this, though. In addition, this is used with
		 * the initial connection wait on the GO as a separate formation
		 * timeout and as such, expected to be hit after the initial WPS
		 * provisioning step.
		 */
		wpa_printf(MSG_DEBUG, "P2P: Canceled P2P group formation timeout on data connection");
	}
	if (!wpa_s->p2p_go_group_formation_completed) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Marking group formation completed on GO on first data connection");
		wpa_s->p2p_go_group_formation_completed = 1;
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
		wpa_s->p2p_in_invitation = 0;
	}
	wpa_s->global->p2p_go_wait_client.sec = 0;
	if (addr == NULL)
		return;
	wpas_p2p_add_persistent_group_client(wpa_s, addr);
}


static void wpas_p2p_fallback_to_go_neg(struct wpa_supplicant *wpa_s,
					int group_added)
{
	struct wpa_supplicant *group = wpa_s;
	if (wpa_s->global->p2p_group_formation)
		group = wpa_s->global->p2p_group_formation;
	wpa_s = wpa_s->parent;
	offchannel_send_action_done(wpa_s);
	if (group_added)
		wpas_p2p_group_delete(group, P2P_GROUP_REMOVAL_SILENT);
	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Fall back to GO Negotiation");
	wpas_p2p_connect(wpa_s, wpa_s->pending_join_dev_addr, wpa_s->p2p_pin,
			 wpa_s->p2p_wps_method, wpa_s->p2p_persistent_group, 0,
			 0, 0, wpa_s->p2p_go_intent, wpa_s->p2p_connect_freq,
			 wpa_s->p2p_persistent_id,
			 wpa_s->p2p_pd_before_go_neg,
			 wpa_s->p2p_go_ht40,
			 wpa_s->p2p_go_vht);
}


int wpas_p2p_scan_no_go_seen(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->p2p_fallback_to_go_neg ||
	    wpa_s->p2p_in_provisioning <= 5)
		return 0;

	if (wpas_p2p_peer_go(wpa_s, wpa_s->pending_join_dev_addr) > 0)
		return 0; /* peer operating as a GO */

	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: GO not found for p2p_connect-auto - "
		"fallback to GO Negotiation");
	wpas_p2p_fallback_to_go_neg(wpa_s, 1);

	return 1;
}


unsigned int wpas_p2p_search_delay(struct wpa_supplicant *wpa_s)
{
	struct wpa_supplicant *ifs;

	if (wpa_s->wpa_state > WPA_SCANNING) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Use %u ms search delay due to "
			"concurrent operation",
			wpa_s->conf->p2p_search_delay);
		return wpa_s->conf->p2p_search_delay;
	}

	dl_list_for_each(ifs, &wpa_s->radio->ifaces, struct wpa_supplicant,
			 radio_list) {
		if (ifs != wpa_s && ifs->wpa_state > WPA_SCANNING) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Use %u ms search "
				"delay due to concurrent operation on "
				"interface %s",
				wpa_s->conf->p2p_search_delay,
				ifs->ifname);
			return wpa_s->conf->p2p_search_delay;
		}
	}

	return 0;
}


static int wpas_p2p_remove_psk_entry(struct wpa_supplicant *wpa_s,
				     struct wpa_ssid *s, const u8 *addr,
				     int iface_addr)
{
	struct psk_list_entry *psk, *tmp;
	int changed = 0;

	dl_list_for_each_safe(psk, tmp, &s->psk_list, struct psk_list_entry,
			      list) {
		if ((iface_addr && !psk->p2p &&
		     os_memcmp(addr, psk->addr, ETH_ALEN) == 0) ||
		    (!iface_addr && psk->p2p &&
		     os_memcmp(addr, psk->addr, ETH_ALEN) == 0)) {
			wpa_dbg(wpa_s, MSG_DEBUG,
				"P2P: Remove persistent group PSK list entry for "
				MACSTR " p2p=%u",
				MAC2STR(psk->addr), psk->p2p);
			dl_list_del(&psk->list);
			os_free(psk);
			changed++;
		}
	}

	return changed;
}


void wpas_p2p_new_psk_cb(struct wpa_supplicant *wpa_s, const u8 *mac_addr,
			 const u8 *p2p_dev_addr,
			 const u8 *psk, size_t psk_len)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	struct wpa_ssid *persistent;
	struct psk_list_entry *p, *last;

	if (psk_len != sizeof(p->psk))
		return;

	if (p2p_dev_addr) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: New PSK for addr=" MACSTR
			" p2p_dev_addr=" MACSTR,
			MAC2STR(mac_addr), MAC2STR(p2p_dev_addr));
		if (is_zero_ether_addr(p2p_dev_addr))
			p2p_dev_addr = NULL;
	} else {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: New PSK for addr=" MACSTR,
			MAC2STR(mac_addr));
	}

	if (ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: new_psk_cb during group formation");
		/* To be added to persistent group once created */
		if (wpa_s->global->add_psk == NULL) {
			wpa_s->global->add_psk = os_zalloc(sizeof(*p));
			if (wpa_s->global->add_psk == NULL)
				return;
		}
		p = wpa_s->global->add_psk;
		if (p2p_dev_addr) {
			p->p2p = 1;
			os_memcpy(p->addr, p2p_dev_addr, ETH_ALEN);
		} else {
			p->p2p = 0;
			os_memcpy(p->addr, mac_addr, ETH_ALEN);
		}
		os_memcpy(p->psk, psk, psk_len);
		return;
	}

	if (ssid->mode != WPAS_MODE_P2P_GO || !ssid->p2p_persistent_group) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Ignore new_psk_cb on not-persistent GO");
		return;
	}

	persistent = wpas_p2p_get_persistent(wpa_s->parent, NULL, ssid->ssid,
					     ssid->ssid_len);
	if (!persistent) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Could not find persistent group information to store the new PSK");
		return;
	}

	p = os_zalloc(sizeof(*p));
	if (p == NULL)
		return;
	if (p2p_dev_addr) {
		p->p2p = 1;
		os_memcpy(p->addr, p2p_dev_addr, ETH_ALEN);
	} else {
		p->p2p = 0;
		os_memcpy(p->addr, mac_addr, ETH_ALEN);
	}
	os_memcpy(p->psk, psk, psk_len);

	if (dl_list_len(&persistent->psk_list) > P2P_MAX_STORED_CLIENTS &&
	    (last = dl_list_last(&persistent->psk_list,
				 struct psk_list_entry, list))) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Remove oldest PSK entry for "
			MACSTR " (p2p=%u) to make room for a new one",
			MAC2STR(last->addr), last->p2p);
		dl_list_del(&last->list);
		os_free(last);
	}

	wpas_p2p_remove_psk_entry(wpa_s->parent, persistent,
				  p2p_dev_addr ? p2p_dev_addr : mac_addr,
				  p2p_dev_addr == NULL);
	if (p2p_dev_addr) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Add new PSK for p2p_dev_addr="
			MACSTR, MAC2STR(p2p_dev_addr));
	} else {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Add new PSK for addr=" MACSTR,
			MAC2STR(mac_addr));
	}
	dl_list_add(&persistent->psk_list, &p->list);

	if (wpa_s->parent->conf->update_config &&
	    wpa_config_write(wpa_s->parent->confname, wpa_s->parent->conf))
		wpa_printf(MSG_DEBUG, "P2P: Failed to update configuration");
}


static void wpas_p2p_remove_psk(struct wpa_supplicant *wpa_s,
				struct wpa_ssid *s, const u8 *addr,
				int iface_addr)
{
	int res;

	res = wpas_p2p_remove_psk_entry(wpa_s, s, addr, iface_addr);
	if (res > 0 && wpa_s->conf->update_config &&
	    wpa_config_write(wpa_s->confname, wpa_s->conf))
		wpa_dbg(wpa_s, MSG_DEBUG,
			"P2P: Failed to update configuration");
}


static void wpas_p2p_remove_client_go(struct wpa_supplicant *wpa_s,
				      const u8 *peer, int iface_addr)
{
	struct hostapd_data *hapd;
	struct hostapd_wpa_psk *psk, *prev, *rem;
	struct sta_info *sta;

	if (wpa_s->ap_iface == NULL || wpa_s->current_ssid == NULL ||
	    wpa_s->current_ssid->mode != WPAS_MODE_P2P_GO)
		return;

	/* Remove per-station PSK entry */
	hapd = wpa_s->ap_iface->bss[0];
	prev = NULL;
	psk = hapd->conf->ssid.wpa_psk;
	while (psk) {
		if ((iface_addr && os_memcmp(peer, psk->addr, ETH_ALEN) == 0) ||
		    (!iface_addr &&
		     os_memcmp(peer, psk->p2p_dev_addr, ETH_ALEN) == 0)) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Remove operating group PSK entry for "
				MACSTR " iface_addr=%d",
				MAC2STR(peer), iface_addr);
			if (prev)
				prev->next = psk->next;
			else
				hapd->conf->ssid.wpa_psk = psk->next;
			rem = psk;
			psk = psk->next;
			os_free(rem);
		} else {
			prev = psk;
			psk = psk->next;
		}
	}

	/* Disconnect from group */
	if (iface_addr)
		sta = ap_get_sta(hapd, peer);
	else
		sta = ap_get_sta_p2p(hapd, peer);
	if (sta) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Disconnect peer " MACSTR
			" (iface_addr=%d) from group",
			MAC2STR(peer), iface_addr);
		hostapd_drv_sta_deauth(hapd, sta->addr,
				       WLAN_REASON_DEAUTH_LEAVING);
		ap_sta_deauthenticate(hapd, sta, WLAN_REASON_DEAUTH_LEAVING);
	}
}


void wpas_p2p_remove_client(struct wpa_supplicant *wpa_s, const u8 *peer,
			    int iface_addr)
{
	struct wpa_ssid *s;
	struct wpa_supplicant *w;

	wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Remove client " MACSTR, MAC2STR(peer));

	/* Remove from any persistent group */
	for (s = wpa_s->parent->conf->ssid; s; s = s->next) {
		if (s->disabled != 2 || s->mode != WPAS_MODE_P2P_GO)
			continue;
		if (!iface_addr)
			wpas_remove_persistent_peer(wpa_s, s, peer, 0);
		wpas_p2p_remove_psk(wpa_s->parent, s, peer, iface_addr);
	}

	/* Remove from any operating group */
	for (w = wpa_s->global->ifaces; w; w = w->next)
		wpas_p2p_remove_client_go(w, peer, iface_addr);
}


static void wpas_p2p_psk_failure_removal(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_PSK_FAILURE);
}


static void wpas_p2p_group_freq_conflict(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	wpa_printf(MSG_DEBUG, "P2P: Frequency conflict - terminate group");
	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_FREQ_CONFLICT);
}


int wpas_p2p_handle_frequency_conflicts(struct wpa_supplicant *wpa_s, int freq,
					struct wpa_ssid *ssid)
{
	struct wpa_supplicant *iface;

	for (iface = wpa_s->global->ifaces; iface; iface = iface->next) {
		if (!iface->current_ssid ||
		    iface->current_ssid->frequency == freq ||
		    (iface->p2p_group_interface == NOT_P2P_GROUP_INTERFACE &&
		     !iface->current_ssid->p2p_group))
			continue;

		/* Remove the connection with least priority */
		if (!wpas_is_p2p_prioritized(iface)) {
			/* STA connection has priority over existing
			 * P2P connection, so remove the interface. */
			wpa_printf(MSG_DEBUG, "P2P: Removing P2P connection due to single channel concurrent mode frequency conflict");
			eloop_register_timeout(0, 0,
					       wpas_p2p_group_freq_conflict,
					       iface, NULL);
			/* If connection in progress is P2P connection, do not
			 * proceed for the connection. */
			if (wpa_s == iface)
				return -1;
			else
				return 0;
		} else {
			/* P2P connection has priority, disable the STA network
			 */
			wpa_supplicant_disable_network(wpa_s->global->ifaces,
						       ssid);
			wpa_msg(wpa_s->global->ifaces, MSG_INFO,
				WPA_EVENT_FREQ_CONFLICT " id=%d", ssid->id);
			os_memset(wpa_s->global->ifaces->pending_bssid, 0,
				  ETH_ALEN);
			/* If P2P connection is in progress, continue
			 * connecting...*/
			if (wpa_s == iface)
				return 0;
			else
				return -1;
		}
	}

	return 0;
}


int wpas_p2p_4way_hs_failed(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (ssid == NULL || !ssid->p2p_group)
		return 0;

	if (wpa_s->p2p_last_4way_hs_fail &&
	    wpa_s->p2p_last_4way_hs_fail == ssid) {
		u8 go_dev_addr[ETH_ALEN];
		struct wpa_ssid *persistent;

		if (wpas_p2p_persistent_group(wpa_s, go_dev_addr,
					      ssid->ssid,
					      ssid->ssid_len) <= 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Could not determine whether 4-way handshake failures were for a persistent group");
			goto disconnect;
		}

		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Two 4-way handshake failures for a P2P group - go_dev_addr="
			MACSTR, MAC2STR(go_dev_addr));
		persistent = wpas_p2p_get_persistent(wpa_s->parent, go_dev_addr,
						     ssid->ssid,
						     ssid->ssid_len);
		if (persistent == NULL || persistent->mode != WPAS_MODE_INFRA) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: No matching persistent group stored");
			goto disconnect;
		}
		wpa_msg_global(wpa_s->parent, MSG_INFO,
			       P2P_EVENT_PERSISTENT_PSK_FAIL "%d",
			       persistent->id);
	disconnect:
		wpa_s->p2p_last_4way_hs_fail = NULL;
		/*
		 * Remove the group from a timeout to avoid issues with caller
		 * continuing to use the interface if this is on a P2P group
		 * interface.
		 */
		eloop_register_timeout(0, 0, wpas_p2p_psk_failure_removal,
				       wpa_s, NULL);
		return 1;
	}

	wpa_s->p2p_last_4way_hs_fail = ssid;
	return 0;
}


#ifdef CONFIG_WPS_NFC

static struct wpabuf * wpas_p2p_nfc_handover(int ndef, struct wpabuf *wsc,
					     struct wpabuf *p2p)
{
	struct wpabuf *ret;
	size_t wsc_len;

	if (p2p == NULL) {
		wpabuf_free(wsc);
		wpa_printf(MSG_DEBUG, "P2P: No p2p buffer for handover");
		return NULL;
	}

	wsc_len = wsc ? wpabuf_len(wsc) : 0;
	ret = wpabuf_alloc(2 + wsc_len + 2 + wpabuf_len(p2p));
	if (ret == NULL) {
		wpabuf_free(wsc);
		wpabuf_free(p2p);
		return NULL;
	}

	wpabuf_put_be16(ret, wsc_len);
	if (wsc)
		wpabuf_put_buf(ret, wsc);
	wpabuf_put_be16(ret, wpabuf_len(p2p));
	wpabuf_put_buf(ret, p2p);

	wpabuf_free(wsc);
	wpabuf_free(p2p);
	wpa_hexdump_buf(MSG_DEBUG,
			"P2P: Generated NFC connection handover message", ret);

	if (ndef && ret) {
		struct wpabuf *tmp;
		tmp = ndef_build_p2p(ret);
		wpabuf_free(ret);
		if (tmp == NULL) {
			wpa_printf(MSG_DEBUG, "P2P: Failed to NDEF encapsulate handover request");
			return NULL;
		}
		ret = tmp;
	}

	return ret;
}


static int wpas_p2p_cli_freq(struct wpa_supplicant *wpa_s,
			     struct wpa_ssid **ssid, u8 *go_dev_addr)
{
	struct wpa_supplicant *iface;

	if (go_dev_addr)
		os_memset(go_dev_addr, 0, ETH_ALEN);
	if (ssid)
		*ssid = NULL;
	for (iface = wpa_s->global->ifaces; iface; iface = iface->next) {
		if (iface->wpa_state < WPA_ASSOCIATING ||
		    iface->current_ssid == NULL || iface->assoc_freq == 0 ||
		    !iface->current_ssid->p2p_group ||
		    iface->current_ssid->mode != WPAS_MODE_INFRA)
			continue;
		if (ssid)
			*ssid = iface->current_ssid;
		if (go_dev_addr)
			os_memcpy(go_dev_addr, iface->go_dev_addr, ETH_ALEN);
		return iface->assoc_freq;
	}
	return 0;
}


struct wpabuf * wpas_p2p_nfc_handover_req(struct wpa_supplicant *wpa_s,
					  int ndef)
{
	struct wpabuf *wsc, *p2p;
	struct wpa_ssid *ssid;
	u8 go_dev_addr[ETH_ALEN];
	int cli_freq = wpas_p2p_cli_freq(wpa_s, &ssid, go_dev_addr);

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: P2P disabled - cannot build handover request");
		return NULL;
	}

	if (wpa_s->conf->wps_nfc_dh_pubkey == NULL &&
	    wps_nfc_gen_dh(&wpa_s->conf->wps_nfc_dh_pubkey,
			   &wpa_s->conf->wps_nfc_dh_privkey) < 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: No DH key available for handover request");
		return NULL;
	}

	if (cli_freq == 0) {
		wsc = wps_build_nfc_handover_req_p2p(
			wpa_s->parent->wps, wpa_s->conf->wps_nfc_dh_pubkey);
	} else
		wsc = NULL;
	p2p = p2p_build_nfc_handover_req(wpa_s->global->p2p, cli_freq,
					 go_dev_addr, ssid ? ssid->ssid : NULL,
					 ssid ? ssid->ssid_len : 0);

	return wpas_p2p_nfc_handover(ndef, wsc, p2p);
}


struct wpabuf * wpas_p2p_nfc_handover_sel(struct wpa_supplicant *wpa_s,
					  int ndef, int tag)
{
	struct wpabuf *wsc, *p2p;
	struct wpa_ssid *ssid;
	u8 go_dev_addr[ETH_ALEN];
	int cli_freq = wpas_p2p_cli_freq(wpa_s, &ssid, go_dev_addr);

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return NULL;

	if (!tag && wpa_s->conf->wps_nfc_dh_pubkey == NULL &&
	    wps_nfc_gen_dh(&wpa_s->conf->wps_nfc_dh_pubkey,
			   &wpa_s->conf->wps_nfc_dh_privkey) < 0)
		return NULL;

	if (cli_freq == 0) {
		wsc = wps_build_nfc_handover_sel_p2p(
			wpa_s->parent->wps,
			tag ? wpa_s->conf->wps_nfc_dev_pw_id :
			DEV_PW_NFC_CONNECTION_HANDOVER,
			wpa_s->conf->wps_nfc_dh_pubkey,
			tag ? wpa_s->conf->wps_nfc_dev_pw : NULL);
	} else
		wsc = NULL;
	p2p = p2p_build_nfc_handover_sel(wpa_s->global->p2p, cli_freq,
					 go_dev_addr, ssid ? ssid->ssid : NULL,
					 ssid ? ssid->ssid_len : 0);

	return wpas_p2p_nfc_handover(ndef, wsc, p2p);
}


static int wpas_p2p_nfc_join_group(struct wpa_supplicant *wpa_s,
				   struct p2p_nfc_params *params)
{
	wpa_printf(MSG_DEBUG, "P2P: Initiate join-group based on NFC "
		   "connection handover (freq=%d)",
		   params->go_freq);

	if (params->go_freq && params->go_ssid_len) {
		wpa_s->p2p_wps_method = WPS_NFC;
		wpa_s->pending_join_wps_method = WPS_NFC;
		os_memset(wpa_s->pending_join_iface_addr, 0, ETH_ALEN);
		os_memcpy(wpa_s->pending_join_dev_addr, params->go_dev_addr,
			  ETH_ALEN);
		return wpas_p2p_join_start(wpa_s, params->go_freq,
					   params->go_ssid,
					   params->go_ssid_len);
	}

	return wpas_p2p_connect(wpa_s, params->peer->p2p_device_addr, NULL,
				WPS_NFC, 0, 0, 1, 0, wpa_s->conf->p2p_go_intent,
				params->go_freq, -1, 0, 1, 1);
}


static int wpas_p2p_nfc_auth_join(struct wpa_supplicant *wpa_s,
				  struct p2p_nfc_params *params, int tag)
{
	int res, persistent;
	struct wpa_ssid *ssid;

	wpa_printf(MSG_DEBUG, "P2P: Authorize join-group based on NFC "
		   "connection handover");
	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		ssid = wpa_s->current_ssid;
		if (ssid == NULL)
			continue;
		if (ssid->mode != WPAS_MODE_P2P_GO)
			continue;
		if (wpa_s->ap_iface == NULL)
			continue;
		break;
	}
	if (wpa_s == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Could not find GO interface");
		return -1;
	}

	if (wpa_s->parent->p2p_oob_dev_pw_id !=
	    DEV_PW_NFC_CONNECTION_HANDOVER &&
	    !wpa_s->parent->p2p_oob_dev_pw) {
		wpa_printf(MSG_DEBUG, "P2P: No NFC Dev Pw known");
		return -1;
	}
	res = wpas_ap_wps_add_nfc_pw(
		wpa_s, wpa_s->parent->p2p_oob_dev_pw_id,
		wpa_s->parent->p2p_oob_dev_pw,
		wpa_s->parent->p2p_peer_oob_pk_hash_known ?
		wpa_s->parent->p2p_peer_oob_pubkey_hash : NULL);
	if (res)
		return res;

	if (!tag) {
		wpa_printf(MSG_DEBUG, "P2P: Negotiated handover - wait for peer to join without invitation");
		return 0;
	}

	if (!params->peer ||
	    !(params->peer->dev_capab & P2P_DEV_CAPAB_INVITATION_PROCEDURE))
		return 0;

	wpa_printf(MSG_DEBUG, "P2P: Static handover - invite peer " MACSTR
		   " to join", MAC2STR(params->peer->p2p_device_addr));

	wpa_s->global->p2p_invite_group = wpa_s;
	persistent = ssid->p2p_persistent_group &&
		wpas_p2p_get_persistent(wpa_s->parent,
					params->peer->p2p_device_addr,
					ssid->ssid, ssid->ssid_len);
	wpa_s->parent->pending_invite_ssid_id = -1;

	return p2p_invite(wpa_s->global->p2p, params->peer->p2p_device_addr,
			  P2P_INVITE_ROLE_ACTIVE_GO, wpa_s->own_addr,
			  ssid->ssid, ssid->ssid_len, ssid->frequency,
			  wpa_s->global->p2p_dev_addr, persistent, 0,
			  wpa_s->parent->p2p_oob_dev_pw_id);
}


static int wpas_p2p_nfc_init_go_neg(struct wpa_supplicant *wpa_s,
				    struct p2p_nfc_params *params,
				    int forced_freq)
{
	wpa_printf(MSG_DEBUG, "P2P: Initiate GO Negotiation based on NFC "
		   "connection handover");
	return wpas_p2p_connect(wpa_s, params->peer->p2p_device_addr, NULL,
				WPS_NFC, 0, 0, 0, 0, wpa_s->conf->p2p_go_intent,
				forced_freq, -1, 0, 1, 1);
}


static int wpas_p2p_nfc_resp_go_neg(struct wpa_supplicant *wpa_s,
				    struct p2p_nfc_params *params,
				    int forced_freq)
{
	int res;

	wpa_printf(MSG_DEBUG, "P2P: Authorize GO Negotiation based on NFC "
		   "connection handover");
	res = wpas_p2p_connect(wpa_s, params->peer->p2p_device_addr, NULL,
			       WPS_NFC, 0, 0, 0, 1, wpa_s->conf->p2p_go_intent,
			       forced_freq, -1, 0, 1, 1);
	if (res)
		return res;

	res = wpas_p2p_listen(wpa_s, 60);
	if (res) {
		p2p_unauthorize(wpa_s->global->p2p,
				params->peer->p2p_device_addr);
	}

	return res;
}


static int wpas_p2p_nfc_connection_handover(struct wpa_supplicant *wpa_s,
					    const struct wpabuf *data,
					    int sel, int tag, int forced_freq)
{
	const u8 *pos, *end;
	u16 len, id;
	struct p2p_nfc_params params;
	int res;

	os_memset(&params, 0, sizeof(params));
	params.sel = sel;

	wpa_hexdump_buf(MSG_DEBUG, "P2P: Received NFC tag payload", data);

	pos = wpabuf_head(data);
	end = pos + wpabuf_len(data);

	if (end - pos < 2) {
		wpa_printf(MSG_DEBUG, "P2P: Not enough data for Length of WSC "
			   "attributes");
		return -1;
	}
	len = WPA_GET_BE16(pos);
	pos += 2;
	if (pos + len > end) {
		wpa_printf(MSG_DEBUG, "P2P: Not enough data for WSC "
			   "attributes");
		return -1;
	}
	params.wsc_attr = pos;
	params.wsc_len = len;
	pos += len;

	if (end - pos < 2) {
		wpa_printf(MSG_DEBUG, "P2P: Not enough data for Length of P2P "
			   "attributes");
		return -1;
	}
	len = WPA_GET_BE16(pos);
	pos += 2;
	if (pos + len > end) {
		wpa_printf(MSG_DEBUG, "P2P: Not enough data for P2P "
			   "attributes");
		return -1;
	}
	params.p2p_attr = pos;
	params.p2p_len = len;
	pos += len;

	wpa_hexdump(MSG_DEBUG, "P2P: WSC attributes",
		    params.wsc_attr, params.wsc_len);
	wpa_hexdump(MSG_DEBUG, "P2P: P2P attributes",
		    params.p2p_attr, params.p2p_len);
	if (pos < end) {
		wpa_hexdump(MSG_DEBUG,
			    "P2P: Ignored extra data after P2P attributes",
			    pos, end - pos);
	}

	res = p2p_process_nfc_connection_handover(wpa_s->global->p2p, &params);
	if (res)
		return res;

	if (params.next_step == NO_ACTION)
		return 0;

	if (params.next_step == BOTH_GO) {
		wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_NFC_BOTH_GO "peer=" MACSTR,
			MAC2STR(params.peer->p2p_device_addr));
		return 0;
	}

	if (params.next_step == PEER_CLIENT) {
		if (!is_zero_ether_addr(params.go_dev_addr)) {
			wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_NFC_PEER_CLIENT
				"peer=" MACSTR " freq=%d go_dev_addr=" MACSTR
				" ssid=\"%s\"",
				MAC2STR(params.peer->p2p_device_addr),
				params.go_freq,
				MAC2STR(params.go_dev_addr),
				wpa_ssid_txt(params.go_ssid,
					     params.go_ssid_len));
		} else {
			wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_NFC_PEER_CLIENT
				"peer=" MACSTR " freq=%d",
				MAC2STR(params.peer->p2p_device_addr),
				params.go_freq);
		}
		return 0;
	}

	if (wpas_p2p_cli_freq(wpa_s, NULL, NULL)) {
		wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_NFC_WHILE_CLIENT "peer="
			MACSTR, MAC2STR(params.peer->p2p_device_addr));
		return 0;
	}

	wpabuf_free(wpa_s->p2p_oob_dev_pw);
	wpa_s->p2p_oob_dev_pw = NULL;

	if (params.oob_dev_pw_len < WPS_OOB_PUBKEY_HASH_LEN + 2) {
		wpa_printf(MSG_DEBUG, "P2P: No peer OOB Dev Pw "
			   "received");
		return -1;
	}

	id = WPA_GET_BE16(params.oob_dev_pw + WPS_OOB_PUBKEY_HASH_LEN);
	wpa_printf(MSG_DEBUG, "P2P: Peer OOB Dev Pw %u", id);
	wpa_hexdump(MSG_DEBUG, "P2P: Peer OOB Public Key hash",
		    params.oob_dev_pw, WPS_OOB_PUBKEY_HASH_LEN);
	os_memcpy(wpa_s->p2p_peer_oob_pubkey_hash,
		  params.oob_dev_pw, WPS_OOB_PUBKEY_HASH_LEN);
	wpa_s->p2p_peer_oob_pk_hash_known = 1;

	if (tag) {
		if (id < 0x10) {
			wpa_printf(MSG_DEBUG, "P2P: Static handover - invalid "
				   "peer OOB Device Password Id %u", id);
			return -1;
		}
		wpa_printf(MSG_DEBUG, "P2P: Static handover - use peer OOB "
			   "Device Password Id %u", id);
		wpa_hexdump_key(MSG_DEBUG, "P2P: Peer OOB Device Password",
				params.oob_dev_pw + WPS_OOB_PUBKEY_HASH_LEN + 2,
				params.oob_dev_pw_len -
				WPS_OOB_PUBKEY_HASH_LEN - 2);
		wpa_s->p2p_oob_dev_pw_id = id;
		wpa_s->p2p_oob_dev_pw = wpabuf_alloc_copy(
			params.oob_dev_pw + WPS_OOB_PUBKEY_HASH_LEN + 2,
			params.oob_dev_pw_len -
			WPS_OOB_PUBKEY_HASH_LEN - 2);
		if (wpa_s->p2p_oob_dev_pw == NULL)
			return -1;

		if (wpa_s->conf->wps_nfc_dh_pubkey == NULL &&
		    wps_nfc_gen_dh(&wpa_s->conf->wps_nfc_dh_pubkey,
				   &wpa_s->conf->wps_nfc_dh_privkey) < 0)
			return -1;
	} else {
		wpa_printf(MSG_DEBUG, "P2P: Using abbreviated WPS handshake "
			   "without Device Password");
		wpa_s->p2p_oob_dev_pw_id = DEV_PW_NFC_CONNECTION_HANDOVER;
	}

	switch (params.next_step) {
	case NO_ACTION:
	case BOTH_GO:
	case PEER_CLIENT:
		/* already covered above */
		return 0;
	case JOIN_GROUP:
		return wpas_p2p_nfc_join_group(wpa_s, &params);
	case AUTH_JOIN:
		return wpas_p2p_nfc_auth_join(wpa_s, &params, tag);
	case INIT_GO_NEG:
		return wpas_p2p_nfc_init_go_neg(wpa_s, &params, forced_freq);
	case RESP_GO_NEG:
		/* TODO: use own OOB Dev Pw */
		return wpas_p2p_nfc_resp_go_neg(wpa_s, &params, forced_freq);
	}

	return -1;
}


int wpas_p2p_nfc_tag_process(struct wpa_supplicant *wpa_s,
			     const struct wpabuf *data, int forced_freq)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return wpas_p2p_nfc_connection_handover(wpa_s, data, 1, 1, forced_freq);
}


int wpas_p2p_nfc_report_handover(struct wpa_supplicant *wpa_s, int init,
				 const struct wpabuf *req,
				 const struct wpabuf *sel, int forced_freq)
{
	struct wpabuf *tmp;
	int ret;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	wpa_printf(MSG_DEBUG, "NFC: P2P connection handover reported");

	wpa_hexdump_ascii(MSG_DEBUG, "NFC: Req",
			  wpabuf_head(req), wpabuf_len(req));
	wpa_hexdump_ascii(MSG_DEBUG, "NFC: Sel",
			  wpabuf_head(sel), wpabuf_len(sel));
	if (forced_freq)
		wpa_printf(MSG_DEBUG, "NFC: Forced freq %d", forced_freq);
	tmp = ndef_parse_p2p(init ? sel : req);
	if (tmp == NULL) {
		wpa_printf(MSG_DEBUG, "P2P: Could not parse NDEF");
		return -1;
	}

	ret = wpas_p2p_nfc_connection_handover(wpa_s, tmp, init, 0,
					       forced_freq);
	wpabuf_free(tmp);

	return ret;
}


int wpas_p2p_nfc_tag_enabled(struct wpa_supplicant *wpa_s, int enabled)
{
	const u8 *if_addr;
	int go_intent = wpa_s->conf->p2p_go_intent;
	struct wpa_supplicant *iface;

	if (wpa_s->global->p2p == NULL)
		return -1;

	if (!enabled) {
		wpa_printf(MSG_DEBUG, "P2P: Disable use of own NFC Tag");
		for (iface = wpa_s->global->ifaces; iface; iface = iface->next)
		{
			if (!iface->ap_iface)
				continue;
			hostapd_wps_nfc_token_disable(iface->ap_iface->bss[0]);
		}
		p2p_set_authorized_oob_dev_pw_id(wpa_s->global->p2p, 0,
						 0, NULL);
		if (wpa_s->p2p_nfc_tag_enabled)
			wpas_p2p_remove_pending_group_interface(wpa_s);
		wpa_s->p2p_nfc_tag_enabled = 0;
		return 0;
	}

	if (wpa_s->global->p2p_disabled)
		return -1;

	if (wpa_s->conf->wps_nfc_dh_pubkey == NULL ||
	    wpa_s->conf->wps_nfc_dh_privkey == NULL ||
	    wpa_s->conf->wps_nfc_dev_pw == NULL ||
	    wpa_s->conf->wps_nfc_dev_pw_id < 0x10) {
		wpa_printf(MSG_DEBUG, "P2P: NFC password token not configured "
			   "to allow static handover cases");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "P2P: Enable use of own NFC Tag");

	wpa_s->p2p_oob_dev_pw_id = wpa_s->conf->wps_nfc_dev_pw_id;
	wpabuf_free(wpa_s->p2p_oob_dev_pw);
	wpa_s->p2p_oob_dev_pw = wpabuf_dup(wpa_s->conf->wps_nfc_dev_pw);
	if (wpa_s->p2p_oob_dev_pw == NULL)
		return -1;
	wpa_s->p2p_peer_oob_pk_hash_known = 0;

	if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_GO ||
	    wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT) {
		/*
		 * P2P Group Interface present and the command came on group
		 * interface, so enable the token for the current interface.
		 */
		wpa_s->create_p2p_iface = 0;
	} else {
		wpa_s->create_p2p_iface = wpas_p2p_create_iface(wpa_s);
	}

	if (wpa_s->create_p2p_iface) {
		enum wpa_driver_if_type iftype;
		/* Prepare to add a new interface for the group */
		iftype = WPA_IF_P2P_GROUP;
		if (go_intent == 15)
			iftype = WPA_IF_P2P_GO;
		if (wpas_p2p_add_group_interface(wpa_s, iftype) < 0) {
			wpa_printf(MSG_ERROR, "P2P: Failed to allocate a new "
				   "interface for the group");
			return -1;
		}

		if_addr = wpa_s->pending_interface_addr;
	} else
		if_addr = wpa_s->own_addr;

	wpa_s->p2p_nfc_tag_enabled = enabled;

	for (iface = wpa_s->global->ifaces; iface; iface = iface->next) {
		struct hostapd_data *hapd;
		if (iface->ap_iface == NULL)
			continue;
		hapd = iface->ap_iface->bss[0];
		wpabuf_free(hapd->conf->wps_nfc_dh_pubkey);
		hapd->conf->wps_nfc_dh_pubkey =
			wpabuf_dup(wpa_s->conf->wps_nfc_dh_pubkey);
		wpabuf_free(hapd->conf->wps_nfc_dh_privkey);
		hapd->conf->wps_nfc_dh_privkey =
			wpabuf_dup(wpa_s->conf->wps_nfc_dh_privkey);
		wpabuf_free(hapd->conf->wps_nfc_dev_pw);
		hapd->conf->wps_nfc_dev_pw =
			wpabuf_dup(wpa_s->conf->wps_nfc_dev_pw);
		hapd->conf->wps_nfc_dev_pw_id = wpa_s->conf->wps_nfc_dev_pw_id;

		if (hostapd_wps_nfc_token_enable(iface->ap_iface->bss[0]) < 0) {
			wpa_dbg(iface, MSG_DEBUG,
				"P2P: Failed to enable NFC Tag for GO");
		}
	}
	p2p_set_authorized_oob_dev_pw_id(
		wpa_s->global->p2p, wpa_s->conf->wps_nfc_dev_pw_id, go_intent,
		if_addr);

	return 0;
}

#endif /* CONFIG_WPS_NFC */


static void wpas_p2p_optimize_listen_channel(struct wpa_supplicant *wpa_s,
					     struct wpa_used_freq_data *freqs,
					     unsigned int num)
{
	u8 curr_chan, cand, chan;
	unsigned int i;

	curr_chan = p2p_get_listen_channel(wpa_s->global->p2p);
	for (i = 0, cand = 0; i < num; i++) {
		ieee80211_freq_to_chan(freqs[i].freq, &chan);
		if (curr_chan == chan) {
			cand = 0;
			break;
		}

		if (chan == 1 || chan == 6 || chan == 11)
			cand = chan;
	}

	if (cand) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"P2P: Update Listen channel to %u baased on operating channel",
			cand);
		p2p_set_listen_channel(wpa_s->global->p2p, 81, cand, 0);
	}
}


void wpas_p2p_indicate_state_change(struct wpa_supplicant *wpa_s)
{
	struct wpa_used_freq_data *freqs;
	unsigned int num = wpa_s->num_multichan_concurrent;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;

	/*
	 * If possible, optimize the Listen channel to be a channel that is
	 * already used by one of the other interfaces.
	 */
	if (!wpa_s->conf->p2p_optimize_listen_chan)
		return;

	if (!wpa_s->current_ssid || wpa_s->wpa_state != WPA_COMPLETED)
		return;

	freqs = os_calloc(num, sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return;

	num = get_shared_radio_freqs_data(wpa_s, freqs, num);

	wpas_p2p_optimize_listen_channel(wpa_s, freqs, num);
	os_free(freqs);
}


void wpas_p2p_deinit_iface(struct wpa_supplicant *wpa_s)
{
	if (wpa_s == wpa_s->parent)
		wpas_p2p_group_remove(wpa_s, "*");
	if (wpa_s == wpa_s->global->p2p_init_wpa_s && wpa_s->global->p2p) {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Disable P2P since removing "
			"the management interface is being removed");
		wpas_p2p_deinit_global(wpa_s->global);
	}
}


void wpas_p2p_ap_deinit(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->ap_iface->bss)
		wpa_s->ap_iface->bss[0]->p2p_group = NULL;
	wpas_p2p_group_deinit(wpa_s);
}
