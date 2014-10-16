/*
 * IEEE 802.11 RSN / WPA Authenticator
 * Copyright (c) 2004-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/state_machine.h"
#include "utils/bitfield.h"
#include "common/ieee802_11_defs.h"
#include "crypto/aes_wrap.h"
#include "crypto/crypto.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/random.h"
#include "eapol_auth/eapol_auth_sm.h"
#include "ap_config.h"
#include "ieee802_11.h"
#include "wpa_auth.h"
#include "pmksa_cache_auth.h"
#include "wpa_auth_i.h"
#include "wpa_auth_ie.h"

#define STATE_MACHINE_DATA struct wpa_state_machine
#define STATE_MACHINE_DEBUG_PREFIX "WPA"
#define STATE_MACHINE_ADDR sm->addr


static void wpa_send_eapol_timeout(void *eloop_ctx, void *timeout_ctx);
static int wpa_sm_step(struct wpa_state_machine *sm);
static int wpa_verify_key_mic(struct wpa_ptk *PTK, u8 *data, size_t data_len);
static void wpa_sm_call_step(void *eloop_ctx, void *timeout_ctx);
static void wpa_group_sm_step(struct wpa_authenticator *wpa_auth,
			      struct wpa_group *group);
static void wpa_request_new_ptk(struct wpa_state_machine *sm);
static int wpa_gtk_update(struct wpa_authenticator *wpa_auth,
			  struct wpa_group *group);
static int wpa_group_config_group_keys(struct wpa_authenticator *wpa_auth,
				       struct wpa_group *group);

static const u32 dot11RSNAConfigGroupUpdateCount = 4;
static const u32 dot11RSNAConfigPairwiseUpdateCount = 4;
static const u32 eapol_key_timeout_first = 100; /* ms */
static const u32 eapol_key_timeout_subseq = 1000; /* ms */
static const u32 eapol_key_timeout_first_group = 500; /* ms */

/* TODO: make these configurable */
static const int dot11RSNAConfigPMKLifetime = 43200;
static const int dot11RSNAConfigPMKReauthThreshold = 70;
static const int dot11RSNAConfigSATimeout = 60;


static inline int wpa_auth_mic_failure_report(
	struct wpa_authenticator *wpa_auth, const u8 *addr)
{
	if (wpa_auth->cb.mic_failure_report)
		return wpa_auth->cb.mic_failure_report(wpa_auth->cb.ctx, addr);
	return 0;
}


static inline void wpa_auth_set_eapol(struct wpa_authenticator *wpa_auth,
				      const u8 *addr, wpa_eapol_variable var,
				      int value)
{
	if (wpa_auth->cb.set_eapol)
		wpa_auth->cb.set_eapol(wpa_auth->cb.ctx, addr, var, value);
}


static inline int wpa_auth_get_eapol(struct wpa_authenticator *wpa_auth,
				     const u8 *addr, wpa_eapol_variable var)
{
	if (wpa_auth->cb.get_eapol == NULL)
		return -1;
	return wpa_auth->cb.get_eapol(wpa_auth->cb.ctx, addr, var);
}


static inline const u8 * wpa_auth_get_psk(struct wpa_authenticator *wpa_auth,
					  const u8 *addr,
					  const u8 *p2p_dev_addr,
					  const u8 *prev_psk)
{
	if (wpa_auth->cb.get_psk == NULL)
		return NULL;
	return wpa_auth->cb.get_psk(wpa_auth->cb.ctx, addr, p2p_dev_addr,
				    prev_psk);
}


static inline int wpa_auth_get_msk(struct wpa_authenticator *wpa_auth,
				   const u8 *addr, u8 *msk, size_t *len)
{
	if (wpa_auth->cb.get_msk == NULL)
		return -1;
	return wpa_auth->cb.get_msk(wpa_auth->cb.ctx, addr, msk, len);
}


static inline int wpa_auth_set_key(struct wpa_authenticator *wpa_auth,
				   int vlan_id,
				   enum wpa_alg alg, const u8 *addr, int idx,
				   u8 *key, size_t key_len)
{
	if (wpa_auth->cb.set_key == NULL)
		return -1;
	return wpa_auth->cb.set_key(wpa_auth->cb.ctx, vlan_id, alg, addr, idx,
				    key, key_len);
}


static inline int wpa_auth_get_seqnum(struct wpa_authenticator *wpa_auth,
				      const u8 *addr, int idx, u8 *seq)
{
	if (wpa_auth->cb.get_seqnum == NULL)
		return -1;
	return wpa_auth->cb.get_seqnum(wpa_auth->cb.ctx, addr, idx, seq);
}


static inline int
wpa_auth_send_eapol(struct wpa_authenticator *wpa_auth, const u8 *addr,
		    const u8 *data, size_t data_len, int encrypt)
{
	if (wpa_auth->cb.send_eapol == NULL)
		return -1;
	return wpa_auth->cb.send_eapol(wpa_auth->cb.ctx, addr, data, data_len,
				       encrypt);
}


int wpa_auth_for_each_sta(struct wpa_authenticator *wpa_auth,
			  int (*cb)(struct wpa_state_machine *sm, void *ctx),
			  void *cb_ctx)
{
	if (wpa_auth->cb.for_each_sta == NULL)
		return 0;
	return wpa_auth->cb.for_each_sta(wpa_auth->cb.ctx, cb, cb_ctx);
}


int wpa_auth_for_each_auth(struct wpa_authenticator *wpa_auth,
			   int (*cb)(struct wpa_authenticator *a, void *ctx),
			   void *cb_ctx)
{
	if (wpa_auth->cb.for_each_auth == NULL)
		return 0;
	return wpa_auth->cb.for_each_auth(wpa_auth->cb.ctx, cb, cb_ctx);
}


void wpa_auth_logger(struct wpa_authenticator *wpa_auth, const u8 *addr,
		     logger_level level, const char *txt)
{
	if (wpa_auth->cb.logger == NULL)
		return;
	wpa_auth->cb.logger(wpa_auth->cb.ctx, addr, level, txt);
}


void wpa_auth_vlogger(struct wpa_authenticator *wpa_auth, const u8 *addr,
		      logger_level level, const char *fmt, ...)
{
	char *format;
	int maxlen;
	va_list ap;

	if (wpa_auth->cb.logger == NULL)
		return;

	maxlen = os_strlen(fmt) + 100;
	format = os_malloc(maxlen);
	if (!format)
		return;

	va_start(ap, fmt);
	vsnprintf(format, maxlen, fmt, ap);
	va_end(ap);

	wpa_auth_logger(wpa_auth, addr, level, format);

	os_free(format);
}


static void wpa_sta_disconnect(struct wpa_authenticator *wpa_auth,
			       const u8 *addr)
{
	if (wpa_auth->cb.disconnect == NULL)
		return;
	wpa_printf(MSG_DEBUG, "wpa_sta_disconnect STA " MACSTR, MAC2STR(addr));
	wpa_auth->cb.disconnect(wpa_auth->cb.ctx, addr,
				WLAN_REASON_PREV_AUTH_NOT_VALID);
}


static int wpa_use_aes_cmac(struct wpa_state_machine *sm)
{
	int ret = 0;
#ifdef CONFIG_IEEE80211R
	if (wpa_key_mgmt_ft(sm->wpa_key_mgmt))
		ret = 1;
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IEEE80211W
	if (wpa_key_mgmt_sha256(sm->wpa_key_mgmt))
		ret = 1;
#endif /* CONFIG_IEEE80211W */
	if (sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN)
		ret = 1;
	return ret;
}


static void wpa_rekey_gmk(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_authenticator *wpa_auth = eloop_ctx;

	if (random_get_bytes(wpa_auth->group->GMK, WPA_GMK_LEN)) {
		wpa_printf(MSG_ERROR, "Failed to get random data for WPA "
			   "initialization.");
	} else {
		wpa_auth_logger(wpa_auth, NULL, LOGGER_DEBUG, "GMK rekeyd");
		wpa_hexdump_key(MSG_DEBUG, "GMK",
				wpa_auth->group->GMK, WPA_GMK_LEN);
	}

	if (wpa_auth->conf.wpa_gmk_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_gmk_rekey, 0,
				       wpa_rekey_gmk, wpa_auth, NULL);
	}
}


static void wpa_rekey_gtk(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_authenticator *wpa_auth = eloop_ctx;
	struct wpa_group *group;

	wpa_auth_logger(wpa_auth, NULL, LOGGER_DEBUG, "rekeying GTK");
	for (group = wpa_auth->group; group; group = group->next) {
		group->GTKReKey = TRUE;
		do {
			group->changed = FALSE;
			wpa_group_sm_step(wpa_auth, group);
		} while (group->changed);
	}

	if (wpa_auth->conf.wpa_group_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_group_rekey,
				       0, wpa_rekey_gtk, wpa_auth, NULL);
	}
}


static void wpa_rekey_ptk(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_authenticator *wpa_auth = eloop_ctx;
	struct wpa_state_machine *sm = timeout_ctx;

	wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG, "rekeying PTK");
	wpa_request_new_ptk(sm);
	wpa_sm_step(sm);
}


static int wpa_auth_pmksa_clear_cb(struct wpa_state_machine *sm, void *ctx)
{
	if (sm->pmksa == ctx)
		sm->pmksa = NULL;
	return 0;
}


static void wpa_auth_pmksa_free_cb(struct rsn_pmksa_cache_entry *entry,
				   void *ctx)
{
	struct wpa_authenticator *wpa_auth = ctx;
	wpa_auth_for_each_sta(wpa_auth, wpa_auth_pmksa_clear_cb, entry);
}


static int wpa_group_init_gmk_and_counter(struct wpa_authenticator *wpa_auth,
					  struct wpa_group *group)
{
	u8 buf[ETH_ALEN + 8 + sizeof(unsigned long)];
	u8 rkey[32];
	unsigned long ptr;

	if (random_get_bytes(group->GMK, WPA_GMK_LEN) < 0)
		return -1;
	wpa_hexdump_key(MSG_DEBUG, "GMK", group->GMK, WPA_GMK_LEN);

	/*
	 * Counter = PRF-256(Random number, "Init Counter",
	 *                   Local MAC Address || Time)
	 */
	os_memcpy(buf, wpa_auth->addr, ETH_ALEN);
	wpa_get_ntp_timestamp(buf + ETH_ALEN);
	ptr = (unsigned long) group;
	os_memcpy(buf + ETH_ALEN + 8, &ptr, sizeof(ptr));
	if (random_get_bytes(rkey, sizeof(rkey)) < 0)
		return -1;

	if (sha1_prf(rkey, sizeof(rkey), "Init Counter", buf, sizeof(buf),
		     group->Counter, WPA_NONCE_LEN) < 0)
		return -1;
	wpa_hexdump_key(MSG_DEBUG, "Key Counter",
			group->Counter, WPA_NONCE_LEN);

	return 0;
}


static struct wpa_group * wpa_group_init(struct wpa_authenticator *wpa_auth,
					 int vlan_id, int delay_init)
{
	struct wpa_group *group;

	group = os_zalloc(sizeof(struct wpa_group));
	if (group == NULL)
		return NULL;

	group->GTKAuthenticator = TRUE;
	group->vlan_id = vlan_id;
	group->GTK_len = wpa_cipher_key_len(wpa_auth->conf.wpa_group);

	if (random_pool_ready() != 1) {
		wpa_printf(MSG_INFO, "WPA: Not enough entropy in random pool "
			   "for secure operations - update keys later when "
			   "the first station connects");
	}

	/*
	 * Set initial GMK/Counter value here. The actual values that will be
	 * used in negotiations will be set once the first station tries to
	 * connect. This allows more time for collecting additional randomness
	 * on embedded devices.
	 */
	if (wpa_group_init_gmk_and_counter(wpa_auth, group) < 0) {
		wpa_printf(MSG_ERROR, "Failed to get random data for WPA "
			   "initialization.");
		os_free(group);
		return NULL;
	}

	group->GInit = TRUE;
	if (delay_init) {
		wpa_printf(MSG_DEBUG, "WPA: Delay group state machine start "
			   "until Beacon frames have been configured");
		/* Initialization is completed in wpa_init_keys(). */
	} else {
		wpa_group_sm_step(wpa_auth, group);
		group->GInit = FALSE;
		wpa_group_sm_step(wpa_auth, group);
	}

	return group;
}


/**
 * wpa_init - Initialize WPA authenticator
 * @addr: Authenticator address
 * @conf: Configuration for WPA authenticator
 * @cb: Callback functions for WPA authenticator
 * Returns: Pointer to WPA authenticator data or %NULL on failure
 */
struct wpa_authenticator * wpa_init(const u8 *addr,
				    struct wpa_auth_config *conf,
				    struct wpa_auth_callbacks *cb)
{
	struct wpa_authenticator *wpa_auth;

	wpa_auth = os_zalloc(sizeof(struct wpa_authenticator));
	if (wpa_auth == NULL)
		return NULL;
	os_memcpy(wpa_auth->addr, addr, ETH_ALEN);
	os_memcpy(&wpa_auth->conf, conf, sizeof(*conf));
	os_memcpy(&wpa_auth->cb, cb, sizeof(*cb));

	if (wpa_auth_gen_wpa_ie(wpa_auth)) {
		wpa_printf(MSG_ERROR, "Could not generate WPA IE.");
		os_free(wpa_auth);
		return NULL;
	}

	wpa_auth->group = wpa_group_init(wpa_auth, 0, 1);
	if (wpa_auth->group == NULL) {
		os_free(wpa_auth->wpa_ie);
		os_free(wpa_auth);
		return NULL;
	}

	wpa_auth->pmksa = pmksa_cache_auth_init(wpa_auth_pmksa_free_cb,
						wpa_auth);
	if (wpa_auth->pmksa == NULL) {
		wpa_printf(MSG_ERROR, "PMKSA cache initialization failed.");
		os_free(wpa_auth->wpa_ie);
		os_free(wpa_auth);
		return NULL;
	}

#ifdef CONFIG_IEEE80211R
	wpa_auth->ft_pmk_cache = wpa_ft_pmk_cache_init();
	if (wpa_auth->ft_pmk_cache == NULL) {
		wpa_printf(MSG_ERROR, "FT PMK cache initialization failed.");
		os_free(wpa_auth->wpa_ie);
		pmksa_cache_auth_deinit(wpa_auth->pmksa);
		os_free(wpa_auth);
		return NULL;
	}
#endif /* CONFIG_IEEE80211R */

	if (wpa_auth->conf.wpa_gmk_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_gmk_rekey, 0,
				       wpa_rekey_gmk, wpa_auth, NULL);
	}

	if (wpa_auth->conf.wpa_group_rekey) {
		eloop_register_timeout(wpa_auth->conf.wpa_group_rekey, 0,
				       wpa_rekey_gtk, wpa_auth, NULL);
	}

#ifdef CONFIG_P2P
	if (WPA_GET_BE32(conf->ip_addr_start)) {
		int count = WPA_GET_BE32(conf->ip_addr_end) -
			WPA_GET_BE32(conf->ip_addr_start) + 1;
		if (count > 1000)
			count = 1000;
		if (count > 0)
			wpa_auth->ip_pool = bitfield_alloc(count);
	}
#endif /* CONFIG_P2P */

	return wpa_auth;
}


int wpa_init_keys(struct wpa_authenticator *wpa_auth)
{
	struct wpa_group *group = wpa_auth->group;

	wpa_printf(MSG_DEBUG, "WPA: Start group state machine to set initial "
		   "keys");
	wpa_group_sm_step(wpa_auth, group);
	group->GInit = FALSE;
	wpa_group_sm_step(wpa_auth, group);
	if (group->wpa_group_state == WPA_GROUP_FATAL_FAILURE)
		return -1;
	return 0;
}


/**
 * wpa_deinit - Deinitialize WPA authenticator
 * @wpa_auth: Pointer to WPA authenticator data from wpa_init()
 */
void wpa_deinit(struct wpa_authenticator *wpa_auth)
{
	struct wpa_group *group, *prev;

	eloop_cancel_timeout(wpa_rekey_gmk, wpa_auth, NULL);
	eloop_cancel_timeout(wpa_rekey_gtk, wpa_auth, NULL);

#ifdef CONFIG_PEERKEY
	while (wpa_auth->stsl_negotiations)
		wpa_stsl_remove(wpa_auth, wpa_auth->stsl_negotiations);
#endif /* CONFIG_PEERKEY */

	pmksa_cache_auth_deinit(wpa_auth->pmksa);

#ifdef CONFIG_IEEE80211R
	wpa_ft_pmk_cache_deinit(wpa_auth->ft_pmk_cache);
	wpa_auth->ft_pmk_cache = NULL;
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_P2P
	bitfield_free(wpa_auth->ip_pool);
#endif /* CONFIG_P2P */


	os_free(wpa_auth->wpa_ie);

	group = wpa_auth->group;
	while (group) {
		prev = group;
		group = group->next;
		os_free(prev);
	}

	os_free(wpa_auth);
}


/**
 * wpa_reconfig - Update WPA authenticator configuration
 * @wpa_auth: Pointer to WPA authenticator data from wpa_init()
 * @conf: Configuration for WPA authenticator
 */
int wpa_reconfig(struct wpa_authenticator *wpa_auth,
		 struct wpa_auth_config *conf)
{
	struct wpa_group *group;
	if (wpa_auth == NULL)
		return 0;

	os_memcpy(&wpa_auth->conf, conf, sizeof(*conf));
	if (wpa_auth_gen_wpa_ie(wpa_auth)) {
		wpa_printf(MSG_ERROR, "Could not generate WPA IE.");
		return -1;
	}

	/*
	 * Reinitialize GTK to make sure it is suitable for the new
	 * configuration.
	 */
	group = wpa_auth->group;
	group->GTK_len = wpa_cipher_key_len(wpa_auth->conf.wpa_group);
	group->GInit = TRUE;
	wpa_group_sm_step(wpa_auth, group);
	group->GInit = FALSE;
	wpa_group_sm_step(wpa_auth, group);

	return 0;
}


struct wpa_state_machine *
wpa_auth_sta_init(struct wpa_authenticator *wpa_auth, const u8 *addr,
		  const u8 *p2p_dev_addr)
{
	struct wpa_state_machine *sm;

	if (wpa_auth->group->wpa_group_state == WPA_GROUP_FATAL_FAILURE)
		return NULL;

	sm = os_zalloc(sizeof(struct wpa_state_machine));
	if (sm == NULL)
		return NULL;
	os_memcpy(sm->addr, addr, ETH_ALEN);
	if (p2p_dev_addr)
		os_memcpy(sm->p2p_dev_addr, p2p_dev_addr, ETH_ALEN);

	sm->wpa_auth = wpa_auth;
	sm->group = wpa_auth->group;

	return sm;
}


int wpa_auth_sta_associated(struct wpa_authenticator *wpa_auth,
			    struct wpa_state_machine *sm)
{
	if (wpa_auth == NULL || !wpa_auth->conf.wpa || sm == NULL)
		return -1;

#ifdef CONFIG_IEEE80211R
	if (sm->ft_completed) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG,
				"FT authentication already completed - do not "
				"start 4-way handshake");
		/* Go to PTKINITDONE state to allow GTK rekeying */
		sm->wpa_ptk_state = WPA_PTK_PTKINITDONE;
		return 0;
	}
#endif /* CONFIG_IEEE80211R */

	if (sm->started) {
		os_memset(&sm->key_replay, 0, sizeof(sm->key_replay));
		sm->ReAuthenticationRequest = TRUE;
		return wpa_sm_step(sm);
	}

	wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG,
			"start authentication");
	sm->started = 1;

	sm->Init = TRUE;
	if (wpa_sm_step(sm) == 1)
		return 1; /* should not really happen */
	sm->Init = FALSE;
	sm->AuthenticationRequest = TRUE;
	return wpa_sm_step(sm);
}


void wpa_auth_sta_no_wpa(struct wpa_state_machine *sm)
{
	/* WPA/RSN was not used - clear WPA state. This is needed if the STA
	 * reassociates back to the same AP while the previous entry for the
	 * STA has not yet been removed. */
	if (sm == NULL)
		return;

	sm->wpa_key_mgmt = 0;
}


static void wpa_free_sta_sm(struct wpa_state_machine *sm)
{
#ifdef CONFIG_P2P
	if (WPA_GET_BE32(sm->ip_addr)) {
		u32 start;
		wpa_printf(MSG_DEBUG, "P2P: Free assigned IP "
			   "address %u.%u.%u.%u from " MACSTR,
			   sm->ip_addr[0], sm->ip_addr[1],
			   sm->ip_addr[2], sm->ip_addr[3],
			   MAC2STR(sm->addr));
		start = WPA_GET_BE32(sm->wpa_auth->conf.ip_addr_start);
		bitfield_clear(sm->wpa_auth->ip_pool,
			       WPA_GET_BE32(sm->ip_addr) - start);
	}
#endif /* CONFIG_P2P */
	if (sm->GUpdateStationKeys) {
		sm->group->GKeyDoneStations--;
		sm->GUpdateStationKeys = FALSE;
	}
#ifdef CONFIG_IEEE80211R
	os_free(sm->assoc_resp_ftie);
	wpabuf_free(sm->ft_pending_req_ies);
#endif /* CONFIG_IEEE80211R */
	os_free(sm->last_rx_eapol_key);
	os_free(sm->wpa_ie);
	os_free(sm);
}


void wpa_auth_sta_deinit(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return;

	if (sm->wpa_auth->conf.wpa_strict_rekey && sm->has_GTK) {
		wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
				"strict rekeying - force GTK rekey since STA "
				"is leaving");
		eloop_cancel_timeout(wpa_rekey_gtk, sm->wpa_auth, NULL);
		eloop_register_timeout(0, 500000, wpa_rekey_gtk, sm->wpa_auth,
				       NULL);
	}

	eloop_cancel_timeout(wpa_send_eapol_timeout, sm->wpa_auth, sm);
	sm->pending_1_of_4_timeout = 0;
	eloop_cancel_timeout(wpa_sm_call_step, sm, NULL);
	eloop_cancel_timeout(wpa_rekey_ptk, sm->wpa_auth, sm);
	if (sm->in_step_loop) {
		/* Must not free state machine while wpa_sm_step() is running.
		 * Freeing will be completed in the end of wpa_sm_step(). */
		wpa_printf(MSG_DEBUG, "WPA: Registering pending STA state "
			   "machine deinit for " MACSTR, MAC2STR(sm->addr));
		sm->pending_deinit = 1;
	} else
		wpa_free_sta_sm(sm);
}


static void wpa_request_new_ptk(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return;

	sm->PTKRequest = TRUE;
	sm->PTK_valid = 0;
}


static int wpa_replay_counter_valid(struct wpa_key_replay_counter *ctr,
				    const u8 *replay_counter)
{
	int i;
	for (i = 0; i < RSNA_MAX_EAPOL_RETRIES; i++) {
		if (!ctr[i].valid)
			break;
		if (os_memcmp(replay_counter, ctr[i].counter,
			      WPA_REPLAY_COUNTER_LEN) == 0)
			return 1;
	}
	return 0;
}


static void wpa_replay_counter_mark_invalid(struct wpa_key_replay_counter *ctr,
					    const u8 *replay_counter)
{
	int i;
	for (i = 0; i < RSNA_MAX_EAPOL_RETRIES; i++) {
		if (ctr[i].valid &&
		    (replay_counter == NULL ||
		     os_memcmp(replay_counter, ctr[i].counter,
			       WPA_REPLAY_COUNTER_LEN) == 0))
			ctr[i].valid = FALSE;
	}
}


#ifdef CONFIG_IEEE80211R
static int ft_check_msg_2_of_4(struct wpa_authenticator *wpa_auth,
			       struct wpa_state_machine *sm,
			       struct wpa_eapol_ie_parse *kde)
{
	struct wpa_ie_data ie;
	struct rsn_mdie *mdie;

	if (wpa_parse_wpa_ie_rsn(kde->rsn_ie, kde->rsn_ie_len, &ie) < 0 ||
	    ie.num_pmkid != 1 || ie.pmkid == NULL) {
		wpa_printf(MSG_DEBUG, "FT: No PMKR1Name in "
			   "FT 4-way handshake message 2/4");
		return -1;
	}

	os_memcpy(sm->sup_pmk_r1_name, ie.pmkid, PMKID_LEN);
	wpa_hexdump(MSG_DEBUG, "FT: PMKR1Name from Supplicant",
		    sm->sup_pmk_r1_name, PMKID_LEN);

	if (!kde->mdie || !kde->ftie) {
		wpa_printf(MSG_DEBUG, "FT: No %s in FT 4-way handshake "
			   "message 2/4", kde->mdie ? "FTIE" : "MDIE");
		return -1;
	}

	mdie = (struct rsn_mdie *) (kde->mdie + 2);
	if (kde->mdie[1] < sizeof(struct rsn_mdie) ||
	    os_memcmp(wpa_auth->conf.mobility_domain, mdie->mobility_domain,
		      MOBILITY_DOMAIN_ID_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "FT: MDIE mismatch");
		return -1;
	}

	if (sm->assoc_resp_ftie &&
	    (kde->ftie[1] != sm->assoc_resp_ftie[1] ||
	     os_memcmp(kde->ftie, sm->assoc_resp_ftie,
		       2 + sm->assoc_resp_ftie[1]) != 0)) {
		wpa_printf(MSG_DEBUG, "FT: FTIE mismatch");
		wpa_hexdump(MSG_DEBUG, "FT: FTIE in EAPOL-Key msg 2/4",
			    kde->ftie, kde->ftie_len);
		wpa_hexdump(MSG_DEBUG, "FT: FTIE in (Re)AssocResp",
			    sm->assoc_resp_ftie, 2 + sm->assoc_resp_ftie[1]);
		return -1;
	}

	return 0;
}
#endif /* CONFIG_IEEE80211R */


static int wpa_receive_error_report(struct wpa_authenticator *wpa_auth,
				    struct wpa_state_machine *sm, int group)
{
	/* Supplicant reported a Michael MIC error */
	wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
			 "received EAPOL-Key Error Request "
			 "(STA detected Michael MIC failure (group=%d))",
			 group);

	if (group && wpa_auth->conf.wpa_group != WPA_CIPHER_TKIP) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"ignore Michael MIC failure report since "
				"group cipher is not TKIP");
	} else if (!group && sm->pairwise != WPA_CIPHER_TKIP) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"ignore Michael MIC failure report since "
				"pairwise cipher is not TKIP");
	} else {
		if (wpa_auth_mic_failure_report(wpa_auth, sm->addr) > 0)
			return 1; /* STA entry was removed */
		sm->dot11RSNAStatsTKIPRemoteMICFailures++;
		wpa_auth->dot11RSNAStatsTKIPRemoteMICFailures++;
	}

	/*
	 * Error report is not a request for a new key handshake, but since
	 * Authenticator may do it, let's change the keys now anyway.
	 */
	wpa_request_new_ptk(sm);
	return 0;
}


void wpa_receive(struct wpa_authenticator *wpa_auth,
		 struct wpa_state_machine *sm,
		 u8 *data, size_t data_len)
{
	struct ieee802_1x_hdr *hdr;
	struct wpa_eapol_key *key;
	u16 key_info, key_data_length;
	enum { PAIRWISE_2, PAIRWISE_4, GROUP_2, REQUEST,
	       SMK_M1, SMK_M3, SMK_ERROR } msg;
	char *msgtxt;
	struct wpa_eapol_ie_parse kde;
	int ft;
	const u8 *eapol_key_ie;
	size_t eapol_key_ie_len;

	if (wpa_auth == NULL || !wpa_auth->conf.wpa || sm == NULL)
		return;

	if (data_len < sizeof(*hdr) + sizeof(*key))
		return;

	hdr = (struct ieee802_1x_hdr *) data;
	key = (struct wpa_eapol_key *) (hdr + 1);
	key_info = WPA_GET_BE16(key->key_info);
	key_data_length = WPA_GET_BE16(key->key_data_length);
	wpa_printf(MSG_DEBUG, "WPA: Received EAPOL-Key from " MACSTR
		   " key_info=0x%x type=%u key_data_length=%u",
		   MAC2STR(sm->addr), key_info, key->type, key_data_length);
	if (key_data_length > data_len - sizeof(*hdr) - sizeof(*key)) {
		wpa_printf(MSG_INFO, "WPA: Invalid EAPOL-Key frame - "
			   "key_data overflow (%d > %lu)",
			   key_data_length,
			   (unsigned long) (data_len - sizeof(*hdr) -
					    sizeof(*key)));
		return;
	}

	if (sm->wpa == WPA_VERSION_WPA2) {
		if (key->type == EAPOL_KEY_TYPE_WPA) {
			/*
			 * Some deployed station implementations seem to send
			 * msg 4/4 with incorrect type value in WPA2 mode.
			 */
			wpa_printf(MSG_DEBUG, "Workaround: Allow EAPOL-Key "
				   "with unexpected WPA type in RSN mode");
		} else if (key->type != EAPOL_KEY_TYPE_RSN) {
			wpa_printf(MSG_DEBUG, "Ignore EAPOL-Key with "
				   "unexpected type %d in RSN mode",
				   key->type);
			return;
		}
	} else {
		if (key->type != EAPOL_KEY_TYPE_WPA) {
			wpa_printf(MSG_DEBUG, "Ignore EAPOL-Key with "
				   "unexpected type %d in WPA mode",
				   key->type);
			return;
		}
	}

	wpa_hexdump(MSG_DEBUG, "WPA: Received Key Nonce", key->key_nonce,
		    WPA_NONCE_LEN);
	wpa_hexdump(MSG_DEBUG, "WPA: Received Replay Counter",
		    key->replay_counter, WPA_REPLAY_COUNTER_LEN);

	/* FIX: verify that the EAPOL-Key frame was encrypted if pairwise keys
	 * are set */

	if ((key_info & (WPA_KEY_INFO_SMK_MESSAGE | WPA_KEY_INFO_REQUEST)) ==
	    (WPA_KEY_INFO_SMK_MESSAGE | WPA_KEY_INFO_REQUEST)) {
		if (key_info & WPA_KEY_INFO_ERROR) {
			msg = SMK_ERROR;
			msgtxt = "SMK Error";
		} else {
			msg = SMK_M1;
			msgtxt = "SMK M1";
		}
	} else if (key_info & WPA_KEY_INFO_SMK_MESSAGE) {
		msg = SMK_M3;
		msgtxt = "SMK M3";
	} else if (key_info & WPA_KEY_INFO_REQUEST) {
		msg = REQUEST;
		msgtxt = "Request";
	} else if (!(key_info & WPA_KEY_INFO_KEY_TYPE)) {
		msg = GROUP_2;
		msgtxt = "2/2 Group";
	} else if (key_data_length == 0) {
		msg = PAIRWISE_4;
		msgtxt = "4/4 Pairwise";
	} else {
		msg = PAIRWISE_2;
		msgtxt = "2/4 Pairwise";
	}

	/* TODO: key_info type validation for PeerKey */
	if (msg == REQUEST || msg == PAIRWISE_2 || msg == PAIRWISE_4 ||
	    msg == GROUP_2) {
		u16 ver = key_info & WPA_KEY_INFO_TYPE_MASK;
		if (sm->pairwise == WPA_CIPHER_CCMP ||
		    sm->pairwise == WPA_CIPHER_GCMP) {
			if (wpa_use_aes_cmac(sm) &&
			    sm->wpa_key_mgmt != WPA_KEY_MGMT_OSEN &&
			    ver != WPA_KEY_INFO_TYPE_AES_128_CMAC) {
				wpa_auth_logger(wpa_auth, sm->addr,
						LOGGER_WARNING,
						"advertised support for "
						"AES-128-CMAC, but did not "
						"use it");
				return;
			}

			if (!wpa_use_aes_cmac(sm) &&
			    ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
				wpa_auth_logger(wpa_auth, sm->addr,
						LOGGER_WARNING,
						"did not use HMAC-SHA1-AES "
						"with CCMP/GCMP");
				return;
			}
		}
	}

	if (key_info & WPA_KEY_INFO_REQUEST) {
		if (sm->req_replay_counter_used &&
		    os_memcmp(key->replay_counter, sm->req_replay_counter,
			      WPA_REPLAY_COUNTER_LEN) <= 0) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_WARNING,
					"received EAPOL-Key request with "
					"replayed counter");
			return;
		}
	}

	if (!(key_info & WPA_KEY_INFO_REQUEST) &&
	    !wpa_replay_counter_valid(sm->key_replay, key->replay_counter)) {
		int i;

		if (msg == PAIRWISE_2 &&
		    wpa_replay_counter_valid(sm->prev_key_replay,
					     key->replay_counter) &&
		    sm->wpa_ptk_state == WPA_PTK_PTKINITNEGOTIATING &&
		    os_memcmp(sm->SNonce, key->key_nonce, WPA_NONCE_LEN) != 0)
		{
			/*
			 * Some supplicant implementations (e.g., Windows XP
			 * WZC) update SNonce for each EAPOL-Key 2/4. This
			 * breaks the workaround on accepting any of the
			 * pending requests, so allow the SNonce to be updated
			 * even if we have already sent out EAPOL-Key 3/4.
			 */
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
					 "Process SNonce update from STA "
					 "based on retransmitted EAPOL-Key "
					 "1/4");
			sm->update_snonce = 1;
			wpa_replay_counter_mark_invalid(sm->prev_key_replay,
							key->replay_counter);
			goto continue_processing;
		}

		if (msg == PAIRWISE_2 &&
		    wpa_replay_counter_valid(sm->prev_key_replay,
					     key->replay_counter) &&
		    sm->wpa_ptk_state == WPA_PTK_PTKINITNEGOTIATING) {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
					 "ignore retransmitted EAPOL-Key %s - "
					 "SNonce did not change", msgtxt);
		} else {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
					 "received EAPOL-Key %s with "
					 "unexpected replay counter", msgtxt);
		}
		for (i = 0; i < RSNA_MAX_EAPOL_RETRIES; i++) {
			if (!sm->key_replay[i].valid)
				break;
			wpa_hexdump(MSG_DEBUG, "pending replay counter",
				    sm->key_replay[i].counter,
				    WPA_REPLAY_COUNTER_LEN);
		}
		wpa_hexdump(MSG_DEBUG, "received replay counter",
			    key->replay_counter, WPA_REPLAY_COUNTER_LEN);
		return;
	}

continue_processing:
	switch (msg) {
	case PAIRWISE_2:
		if (sm->wpa_ptk_state != WPA_PTK_PTKSTART &&
		    sm->wpa_ptk_state != WPA_PTK_PTKCALCNEGOTIATING &&
		    (!sm->update_snonce ||
		     sm->wpa_ptk_state != WPA_PTK_PTKINITNEGOTIATING)) {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
					 "received EAPOL-Key msg 2/4 in "
					 "invalid state (%d) - dropped",
					 sm->wpa_ptk_state);
			return;
		}
		random_add_randomness(key->key_nonce, WPA_NONCE_LEN);
		if (sm->group->reject_4way_hs_for_entropy) {
			/*
			 * The system did not have enough entropy to generate
			 * strong random numbers. Reject the first 4-way
			 * handshake(s) and collect some entropy based on the
			 * information from it. Once enough entropy is
			 * available, the next atempt will trigger GMK/Key
			 * Counter update and the station will be allowed to
			 * continue.
			 */
			wpa_printf(MSG_DEBUG, "WPA: Reject 4-way handshake to "
				   "collect more entropy for random number "
				   "generation");
			random_mark_pool_ready();
			wpa_sta_disconnect(wpa_auth, sm->addr);
			return;
		}
		if (wpa_parse_kde_ies((u8 *) (key + 1), key_data_length,
				      &kde) < 0) {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
					 "received EAPOL-Key msg 2/4 with "
					 "invalid Key Data contents");
			return;
		}
		if (kde.rsn_ie) {
			eapol_key_ie = kde.rsn_ie;
			eapol_key_ie_len = kde.rsn_ie_len;
		} else if (kde.osen) {
			eapol_key_ie = kde.osen;
			eapol_key_ie_len = kde.osen_len;
		} else {
			eapol_key_ie = kde.wpa_ie;
			eapol_key_ie_len = kde.wpa_ie_len;
		}
		ft = sm->wpa == WPA_VERSION_WPA2 &&
			wpa_key_mgmt_ft(sm->wpa_key_mgmt);
		if (sm->wpa_ie == NULL ||
		    wpa_compare_rsn_ie(ft,
				       sm->wpa_ie, sm->wpa_ie_len,
				       eapol_key_ie, eapol_key_ie_len)) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"WPA IE from (Re)AssocReq did not "
					"match with msg 2/4");
			if (sm->wpa_ie) {
				wpa_hexdump(MSG_DEBUG, "WPA IE in AssocReq",
					    sm->wpa_ie, sm->wpa_ie_len);
			}
			wpa_hexdump(MSG_DEBUG, "WPA IE in msg 2/4",
				    eapol_key_ie, eapol_key_ie_len);
			/* MLME-DEAUTHENTICATE.request */
			wpa_sta_disconnect(wpa_auth, sm->addr);
			return;
		}
#ifdef CONFIG_IEEE80211R
		if (ft && ft_check_msg_2_of_4(wpa_auth, sm, &kde) < 0) {
			wpa_sta_disconnect(wpa_auth, sm->addr);
			return;
		}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_P2P
		if (kde.ip_addr_req && kde.ip_addr_req[0] &&
		    wpa_auth->ip_pool && WPA_GET_BE32(sm->ip_addr) == 0) {
			int idx;
			wpa_printf(MSG_DEBUG, "P2P: IP address requested in "
				   "EAPOL-Key exchange");
			idx = bitfield_get_first_zero(wpa_auth->ip_pool);
			if (idx >= 0) {
				u32 start = WPA_GET_BE32(wpa_auth->conf.
							 ip_addr_start);
				bitfield_set(wpa_auth->ip_pool, idx);
				WPA_PUT_BE32(sm->ip_addr, start + idx);
				wpa_printf(MSG_DEBUG, "P2P: Assigned IP "
					   "address %u.%u.%u.%u to " MACSTR,
					   sm->ip_addr[0], sm->ip_addr[1],
					   sm->ip_addr[2], sm->ip_addr[3],
					   MAC2STR(sm->addr));
			}
		}
#endif /* CONFIG_P2P */
		break;
	case PAIRWISE_4:
		if (sm->wpa_ptk_state != WPA_PTK_PTKINITNEGOTIATING ||
		    !sm->PTK_valid) {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
					 "received EAPOL-Key msg 4/4 in "
					 "invalid state (%d) - dropped",
					 sm->wpa_ptk_state);
			return;
		}
		break;
	case GROUP_2:
		if (sm->wpa_ptk_group_state != WPA_PTK_GROUP_REKEYNEGOTIATING
		    || !sm->PTK_valid) {
			wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_INFO,
					 "received EAPOL-Key msg 2/2 in "
					 "invalid state (%d) - dropped",
					 sm->wpa_ptk_group_state);
			return;
		}
		break;
#ifdef CONFIG_PEERKEY
	case SMK_M1:
	case SMK_M3:
	case SMK_ERROR:
		if (!wpa_auth->conf.peerkey) {
			wpa_printf(MSG_DEBUG, "RSN: SMK M1/M3/Error, but "
				   "PeerKey use disabled - ignoring message");
			return;
		}
		if (!sm->PTK_valid) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key msg SMK in "
					"invalid state - dropped");
			return;
		}
		break;
#else /* CONFIG_PEERKEY */
	case SMK_M1:
	case SMK_M3:
	case SMK_ERROR:
		return; /* STSL disabled - ignore SMK messages */
#endif /* CONFIG_PEERKEY */
	case REQUEST:
		break;
	}

	wpa_auth_vlogger(wpa_auth, sm->addr, LOGGER_DEBUG,
			 "received EAPOL-Key frame (%s)", msgtxt);

	if (key_info & WPA_KEY_INFO_ACK) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"received invalid EAPOL-Key: Key Ack set");
		return;
	}

	if (!(key_info & WPA_KEY_INFO_MIC)) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
				"received invalid EAPOL-Key: Key MIC not set");
		return;
	}

	sm->MICVerified = FALSE;
	if (sm->PTK_valid && !sm->update_snonce) {
		if (wpa_verify_key_mic(&sm->PTK, data, data_len)) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key with invalid MIC");
			return;
		}
		sm->MICVerified = TRUE;
		eloop_cancel_timeout(wpa_send_eapol_timeout, wpa_auth, sm);
		sm->pending_1_of_4_timeout = 0;
	}

	if (key_info & WPA_KEY_INFO_REQUEST) {
		if (sm->MICVerified) {
			sm->req_replay_counter_used = 1;
			os_memcpy(sm->req_replay_counter, key->replay_counter,
				  WPA_REPLAY_COUNTER_LEN);
		} else {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key request with "
					"invalid MIC");
			return;
		}

		/*
		 * TODO: should decrypt key data field if encryption was used;
		 * even though MAC address KDE is not normally encrypted,
		 * supplicant is allowed to encrypt it.
		 */
		if (msg == SMK_ERROR) {
#ifdef CONFIG_PEERKEY
			wpa_smk_error(wpa_auth, sm, key);
#endif /* CONFIG_PEERKEY */
			return;
		} else if (key_info & WPA_KEY_INFO_ERROR) {
			if (wpa_receive_error_report(
				    wpa_auth, sm,
				    !(key_info & WPA_KEY_INFO_KEY_TYPE)) > 0)
				return; /* STA entry was removed */
		} else if (key_info & WPA_KEY_INFO_KEY_TYPE) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key Request for new "
					"4-Way Handshake");
			wpa_request_new_ptk(sm);
#ifdef CONFIG_PEERKEY
		} else if (msg == SMK_M1) {
			wpa_smk_m1(wpa_auth, sm, key);
#endif /* CONFIG_PEERKEY */
		} else if (key_data_length > 0 &&
			   wpa_parse_kde_ies((const u8 *) (key + 1),
					     key_data_length, &kde) == 0 &&
			   kde.mac_addr) {
		} else {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"received EAPOL-Key Request for GTK "
					"rekeying");
			eloop_cancel_timeout(wpa_rekey_gtk, wpa_auth, NULL);
			wpa_rekey_gtk(wpa_auth, NULL);
		}
	} else {
		/* Do not allow the same key replay counter to be reused. */
		wpa_replay_counter_mark_invalid(sm->key_replay,
						key->replay_counter);

		if (msg == PAIRWISE_2) {
			/*
			 * Maintain a copy of the pending EAPOL-Key frames in
			 * case the EAPOL-Key frame was retransmitted. This is
			 * needed to allow EAPOL-Key msg 2/4 reply to another
			 * pending msg 1/4 to update the SNonce to work around
			 * unexpected supplicant behavior.
			 */
			os_memcpy(sm->prev_key_replay, sm->key_replay,
				  sizeof(sm->key_replay));
		} else {
			os_memset(sm->prev_key_replay, 0,
				  sizeof(sm->prev_key_replay));
		}

		/*
		 * Make sure old valid counters are not accepted anymore and
		 * do not get copied again.
		 */
		wpa_replay_counter_mark_invalid(sm->key_replay, NULL);
	}

#ifdef CONFIG_PEERKEY
	if (msg == SMK_M3) {
		wpa_smk_m3(wpa_auth, sm, key);
		return;
	}
#endif /* CONFIG_PEERKEY */

	os_free(sm->last_rx_eapol_key);
	sm->last_rx_eapol_key = os_malloc(data_len);
	if (sm->last_rx_eapol_key == NULL)
		return;
	os_memcpy(sm->last_rx_eapol_key, data, data_len);
	sm->last_rx_eapol_key_len = data_len;

	sm->rx_eapol_key_secure = !!(key_info & WPA_KEY_INFO_SECURE);
	sm->EAPOLKeyReceived = TRUE;
	sm->EAPOLKeyPairwise = !!(key_info & WPA_KEY_INFO_KEY_TYPE);
	sm->EAPOLKeyRequest = !!(key_info & WPA_KEY_INFO_REQUEST);
	os_memcpy(sm->SNonce, key->key_nonce, WPA_NONCE_LEN);
	wpa_sm_step(sm);
}


static int wpa_gmk_to_gtk(const u8 *gmk, const char *label, const u8 *addr,
			  const u8 *gnonce, u8 *gtk, size_t gtk_len)
{
	u8 data[ETH_ALEN + WPA_NONCE_LEN + 8 + 16];
	u8 *pos;
	int ret = 0;

	/* GTK = PRF-X(GMK, "Group key expansion",
	 *	AA || GNonce || Time || random data)
	 * The example described in the IEEE 802.11 standard uses only AA and
	 * GNonce as inputs here. Add some more entropy since this derivation
	 * is done only at the Authenticator and as such, does not need to be
	 * exactly same.
	 */
	os_memcpy(data, addr, ETH_ALEN);
	os_memcpy(data + ETH_ALEN, gnonce, WPA_NONCE_LEN);
	pos = data + ETH_ALEN + WPA_NONCE_LEN;
	wpa_get_ntp_timestamp(pos);
	pos += 8;
	if (random_get_bytes(pos, 16) < 0)
		ret = -1;

#ifdef CONFIG_IEEE80211W
	sha256_prf(gmk, WPA_GMK_LEN, label, data, sizeof(data), gtk, gtk_len);
#else /* CONFIG_IEEE80211W */
	if (sha1_prf(gmk, WPA_GMK_LEN, label, data, sizeof(data), gtk, gtk_len)
	    < 0)
		ret = -1;
#endif /* CONFIG_IEEE80211W */

	return ret;
}


static void wpa_send_eapol_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_authenticator *wpa_auth = eloop_ctx;
	struct wpa_state_machine *sm = timeout_ctx;

	sm->pending_1_of_4_timeout = 0;
	wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG, "EAPOL-Key timeout");
	sm->TimeoutEvt = TRUE;
	wpa_sm_step(sm);
}


void __wpa_send_eapol(struct wpa_authenticator *wpa_auth,
		      struct wpa_state_machine *sm, int key_info,
		      const u8 *key_rsc, const u8 *nonce,
		      const u8 *kde, size_t kde_len,
		      int keyidx, int encr, int force_version)
{
	struct ieee802_1x_hdr *hdr;
	struct wpa_eapol_key *key;
	size_t len;
	int alg;
	int key_data_len, pad_len = 0;
	u8 *buf, *pos;
	int version, pairwise;
	int i;

	len = sizeof(struct ieee802_1x_hdr) + sizeof(struct wpa_eapol_key);

	if (force_version)
		version = force_version;
	else if (sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN)
		version = WPA_KEY_INFO_TYPE_AKM_DEFINED;
	else if (wpa_use_aes_cmac(sm))
		version = WPA_KEY_INFO_TYPE_AES_128_CMAC;
	else if (sm->pairwise != WPA_CIPHER_TKIP)
		version = WPA_KEY_INFO_TYPE_HMAC_SHA1_AES;
	else
		version = WPA_KEY_INFO_TYPE_HMAC_MD5_RC4;

	pairwise = !!(key_info & WPA_KEY_INFO_KEY_TYPE);

	wpa_printf(MSG_DEBUG, "WPA: Send EAPOL(version=%d secure=%d mic=%d "
		   "ack=%d install=%d pairwise=%d kde_len=%lu keyidx=%d "
		   "encr=%d)",
		   version,
		   (key_info & WPA_KEY_INFO_SECURE) ? 1 : 0,
		   (key_info & WPA_KEY_INFO_MIC) ? 1 : 0,
		   (key_info & WPA_KEY_INFO_ACK) ? 1 : 0,
		   (key_info & WPA_KEY_INFO_INSTALL) ? 1 : 0,
		   pairwise, (unsigned long) kde_len, keyidx, encr);

	key_data_len = kde_len;

	if ((version == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES ||
	     sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN ||
	     version == WPA_KEY_INFO_TYPE_AES_128_CMAC) && encr) {
		pad_len = key_data_len % 8;
		if (pad_len)
			pad_len = 8 - pad_len;
		key_data_len += pad_len + 8;
	}

	len += key_data_len;

	hdr = os_zalloc(len);
	if (hdr == NULL)
		return;
	hdr->version = wpa_auth->conf.eapol_version;
	hdr->type = IEEE802_1X_TYPE_EAPOL_KEY;
	hdr->length = host_to_be16(len  - sizeof(*hdr));
	key = (struct wpa_eapol_key *) (hdr + 1);

	key->type = sm->wpa == WPA_VERSION_WPA2 ?
		EAPOL_KEY_TYPE_RSN : EAPOL_KEY_TYPE_WPA;
	key_info |= version;
	if (encr && sm->wpa == WPA_VERSION_WPA2)
		key_info |= WPA_KEY_INFO_ENCR_KEY_DATA;
	if (sm->wpa != WPA_VERSION_WPA2)
		key_info |= keyidx << WPA_KEY_INFO_KEY_INDEX_SHIFT;
	WPA_PUT_BE16(key->key_info, key_info);

	alg = pairwise ? sm->pairwise : wpa_auth->conf.wpa_group;
	WPA_PUT_BE16(key->key_length, wpa_cipher_key_len(alg));
	if (key_info & WPA_KEY_INFO_SMK_MESSAGE)
		WPA_PUT_BE16(key->key_length, 0);

	/* FIX: STSL: what to use as key_replay_counter? */
	for (i = RSNA_MAX_EAPOL_RETRIES - 1; i > 0; i--) {
		sm->key_replay[i].valid = sm->key_replay[i - 1].valid;
		os_memcpy(sm->key_replay[i].counter,
			  sm->key_replay[i - 1].counter,
			  WPA_REPLAY_COUNTER_LEN);
	}
	inc_byte_array(sm->key_replay[0].counter, WPA_REPLAY_COUNTER_LEN);
	os_memcpy(key->replay_counter, sm->key_replay[0].counter,
		  WPA_REPLAY_COUNTER_LEN);
	sm->key_replay[0].valid = TRUE;

	if (nonce)
		os_memcpy(key->key_nonce, nonce, WPA_NONCE_LEN);

	if (key_rsc)
		os_memcpy(key->key_rsc, key_rsc, WPA_KEY_RSC_LEN);

	if (kde && !encr) {
		os_memcpy(key + 1, kde, kde_len);
		WPA_PUT_BE16(key->key_data_length, kde_len);
	} else if (encr && kde) {
		buf = os_zalloc(key_data_len);
		if (buf == NULL) {
			os_free(hdr);
			return;
		}
		pos = buf;
		os_memcpy(pos, kde, kde_len);
		pos += kde_len;

		if (pad_len)
			*pos++ = 0xdd;

		wpa_hexdump_key(MSG_DEBUG, "Plaintext EAPOL-Key Key Data",
				buf, key_data_len);
		if (version == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES ||
		    sm->wpa_key_mgmt == WPA_KEY_MGMT_OSEN ||
		    version == WPA_KEY_INFO_TYPE_AES_128_CMAC) {
			if (aes_wrap(sm->PTK.kek, 16,
				     (key_data_len - 8) / 8, buf,
				     (u8 *) (key + 1))) {
				os_free(hdr);
				os_free(buf);
				return;
			}
			WPA_PUT_BE16(key->key_data_length, key_data_len);
		} else {
			u8 ek[32];
			os_memcpy(key->key_iv,
				  sm->group->Counter + WPA_NONCE_LEN - 16, 16);
			inc_byte_array(sm->group->Counter, WPA_NONCE_LEN);
			os_memcpy(ek, key->key_iv, 16);
			os_memcpy(ek + 16, sm->PTK.kek, 16);
			os_memcpy(key + 1, buf, key_data_len);
			rc4_skip(ek, 32, 256, (u8 *) (key + 1), key_data_len);
			WPA_PUT_BE16(key->key_data_length, key_data_len);
		}
		os_free(buf);
	}

	if (key_info & WPA_KEY_INFO_MIC) {
		if (!sm->PTK_valid) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG,
					"PTK not valid when sending EAPOL-Key "
					"frame");
			os_free(hdr);
			return;
		}
		wpa_eapol_key_mic(sm->PTK.kck, version, (u8 *) hdr, len,
				  key->key_mic);
#ifdef CONFIG_TESTING_OPTIONS
		if (!pairwise &&
		    wpa_auth->conf.corrupt_gtk_rekey_mic_probability > 0.0 &&
		    drand48() <
		    wpa_auth->conf.corrupt_gtk_rekey_mic_probability) {
			wpa_auth_logger(wpa_auth, sm->addr, LOGGER_INFO,
					"Corrupting group EAPOL-Key Key MIC");
			key->key_mic[0]++;
		}
#endif /* CONFIG_TESTING_OPTIONS */
	}

	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_inc_EapolFramesTx,
			   1);
	wpa_auth_send_eapol(wpa_auth, sm->addr, (u8 *) hdr, len,
			    sm->pairwise_set);
	os_free(hdr);
}


static void wpa_send_eapol(struct wpa_authenticator *wpa_auth,
			   struct wpa_state_machine *sm, int key_info,
			   const u8 *key_rsc, const u8 *nonce,
			   const u8 *kde, size_t kde_len,
			   int keyidx, int encr)
{
	int timeout_ms;
	int pairwise = key_info & WPA_KEY_INFO_KEY_TYPE;
	int ctr;

	if (sm == NULL)
		return;

	__wpa_send_eapol(wpa_auth, sm, key_info, key_rsc, nonce, kde, kde_len,
			 keyidx, encr, 0);

	ctr = pairwise ? sm->TimeoutCtr : sm->GTimeoutCtr;
	if (ctr == 1 && wpa_auth->conf.tx_status)
		timeout_ms = pairwise ? eapol_key_timeout_first :
			eapol_key_timeout_first_group;
	else
		timeout_ms = eapol_key_timeout_subseq;
	if (pairwise && ctr == 1 && !(key_info & WPA_KEY_INFO_MIC))
		sm->pending_1_of_4_timeout = 1;
	wpa_printf(MSG_DEBUG, "WPA: Use EAPOL-Key timeout of %u ms (retry "
		   "counter %d)", timeout_ms, ctr);
	eloop_register_timeout(timeout_ms / 1000, (timeout_ms % 1000) * 1000,
			       wpa_send_eapol_timeout, wpa_auth, sm);
}


static int wpa_verify_key_mic(struct wpa_ptk *PTK, u8 *data, size_t data_len)
{
	struct ieee802_1x_hdr *hdr;
	struct wpa_eapol_key *key;
	u16 key_info;
	int ret = 0;
	u8 mic[16];

	if (data_len < sizeof(*hdr) + sizeof(*key))
		return -1;

	hdr = (struct ieee802_1x_hdr *) data;
	key = (struct wpa_eapol_key *) (hdr + 1);
	key_info = WPA_GET_BE16(key->key_info);
	os_memcpy(mic, key->key_mic, 16);
	os_memset(key->key_mic, 0, 16);
	if (wpa_eapol_key_mic(PTK->kck, key_info & WPA_KEY_INFO_TYPE_MASK,
			      data, data_len, key->key_mic) ||
	    os_memcmp_const(mic, key->key_mic, 16) != 0)
		ret = -1;
	os_memcpy(key->key_mic, mic, 16);
	return ret;
}


void wpa_remove_ptk(struct wpa_state_machine *sm)
{
	sm->PTK_valid = FALSE;
	os_memset(&sm->PTK, 0, sizeof(sm->PTK));
	wpa_auth_set_key(sm->wpa_auth, 0, WPA_ALG_NONE, sm->addr, 0, NULL, 0);
	sm->pairwise_set = FALSE;
	eloop_cancel_timeout(wpa_rekey_ptk, sm->wpa_auth, sm);
}


int wpa_auth_sm_event(struct wpa_state_machine *sm, wpa_event event)
{
	int remove_ptk = 1;

	if (sm == NULL)
		return -1;

	wpa_auth_vlogger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
			 "event %d notification", event);

	switch (event) {
	case WPA_AUTH:
	case WPA_ASSOC:
		break;
	case WPA_DEAUTH:
	case WPA_DISASSOC:
		sm->DeauthenticationRequest = TRUE;
		break;
	case WPA_REAUTH:
	case WPA_REAUTH_EAPOL:
		if (!sm->started) {
			/*
			 * When using WPS, we may end up here if the STA
			 * manages to re-associate without the previous STA
			 * entry getting removed. Consequently, we need to make
			 * sure that the WPA state machines gets initialized
			 * properly at this point.
			 */
			wpa_printf(MSG_DEBUG, "WPA state machine had not been "
				   "started - initialize now");
			sm->started = 1;
			sm->Init = TRUE;
			if (wpa_sm_step(sm) == 1)
				return 1; /* should not really happen */
			sm->Init = FALSE;
			sm->AuthenticationRequest = TRUE;
			break;
		}
		if (sm->GUpdateStationKeys) {
			/*
			 * Reauthentication cancels the pending group key
			 * update for this STA.
			 */
			sm->group->GKeyDoneStations--;
			sm->GUpdateStationKeys = FALSE;
			sm->PtkGroupInit = TRUE;
		}
		sm->ReAuthenticationRequest = TRUE;
		break;
	case WPA_ASSOC_FT:
#ifdef CONFIG_IEEE80211R
		wpa_printf(MSG_DEBUG, "FT: Retry PTK configuration "
			   "after association");
		wpa_ft_install_ptk(sm);

		/* Using FT protocol, not WPA auth state machine */
		sm->ft_completed = 1;
		return 0;
#else /* CONFIG_IEEE80211R */
		break;
#endif /* CONFIG_IEEE80211R */
	}

#ifdef CONFIG_IEEE80211R
	sm->ft_completed = 0;
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211W
	if (sm->mgmt_frame_prot && event == WPA_AUTH)
		remove_ptk = 0;
#endif /* CONFIG_IEEE80211W */

	if (remove_ptk) {
		sm->PTK_valid = FALSE;
		os_memset(&sm->PTK, 0, sizeof(sm->PTK));

		if (event != WPA_REAUTH_EAPOL)
			wpa_remove_ptk(sm);
	}

	return wpa_sm_step(sm);
}


SM_STATE(WPA_PTK, INITIALIZE)
{
	SM_ENTRY_MA(WPA_PTK, INITIALIZE, wpa_ptk);
	if (sm->Init) {
		/* Init flag is not cleared here, so avoid busy
		 * loop by claiming nothing changed. */
		sm->changed = FALSE;
	}

	sm->keycount = 0;
	if (sm->GUpdateStationKeys)
		sm->group->GKeyDoneStations--;
	sm->GUpdateStationKeys = FALSE;
	if (sm->wpa == WPA_VERSION_WPA)
		sm->PInitAKeys = FALSE;
	if (1 /* Unicast cipher supported AND (ESS OR ((IBSS or WDS) and
	       * Local AA > Remote AA)) */) {
		sm->Pair = TRUE;
	}
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_portEnabled, 0);
	wpa_remove_ptk(sm);
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_portValid, 0);
	sm->TimeoutCtr = 0;
	if (wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt)) {
		wpa_auth_set_eapol(sm->wpa_auth, sm->addr,
				   WPA_EAPOL_authorized, 0);
	}
}


SM_STATE(WPA_PTK, DISCONNECT)
{
	SM_ENTRY_MA(WPA_PTK, DISCONNECT, wpa_ptk);
	sm->Disconnect = FALSE;
	wpa_sta_disconnect(sm->wpa_auth, sm->addr);
}


SM_STATE(WPA_PTK, DISCONNECTED)
{
	SM_ENTRY_MA(WPA_PTK, DISCONNECTED, wpa_ptk);
	sm->DeauthenticationRequest = FALSE;
}


SM_STATE(WPA_PTK, AUTHENTICATION)
{
	SM_ENTRY_MA(WPA_PTK, AUTHENTICATION, wpa_ptk);
	os_memset(&sm->PTK, 0, sizeof(sm->PTK));
	sm->PTK_valid = FALSE;
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_portControl_Auto,
			   1);
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_portEnabled, 1);
	sm->AuthenticationRequest = FALSE;
}


static void wpa_group_ensure_init(struct wpa_authenticator *wpa_auth,
				  struct wpa_group *group)
{
	if (group->first_sta_seen)
		return;
	/*
	 * System has run bit further than at the time hostapd was started
	 * potentially very early during boot up. This provides better chances
	 * of collecting more randomness on embedded systems. Re-initialize the
	 * GMK and Counter here to improve their strength if there was not
	 * enough entropy available immediately after system startup.
	 */
	wpa_printf(MSG_DEBUG, "WPA: Re-initialize GMK/Counter on first "
		   "station");
	if (random_pool_ready() != 1) {
		wpa_printf(MSG_INFO, "WPA: Not enough entropy in random pool "
			   "to proceed - reject first 4-way handshake");
		group->reject_4way_hs_for_entropy = TRUE;
	} else {
		group->first_sta_seen = TRUE;
		group->reject_4way_hs_for_entropy = FALSE;
	}

	wpa_group_init_gmk_and_counter(wpa_auth, group);
	wpa_gtk_update(wpa_auth, group);
	wpa_group_config_group_keys(wpa_auth, group);
}


SM_STATE(WPA_PTK, AUTHENTICATION2)
{
	SM_ENTRY_MA(WPA_PTK, AUTHENTICATION2, wpa_ptk);

	wpa_group_ensure_init(sm->wpa_auth, sm->group);
	sm->ReAuthenticationRequest = FALSE;

	/*
	 * Definition of ANonce selection in IEEE Std 802.11i-2004 is somewhat
	 * ambiguous. The Authenticator state machine uses a counter that is
	 * incremented by one for each 4-way handshake. However, the security
	 * analysis of 4-way handshake points out that unpredictable nonces
	 * help in preventing precomputation attacks. Instead of the state
	 * machine definition, use an unpredictable nonce value here to provide
	 * stronger protection against potential precomputation attacks.
	 */
	if (random_get_bytes(sm->ANonce, WPA_NONCE_LEN)) {
		wpa_printf(MSG_ERROR, "WPA: Failed to get random data for "
			   "ANonce.");
		sm->Disconnect = TRUE;
		return;
	}
	wpa_hexdump(MSG_DEBUG, "WPA: Assign ANonce", sm->ANonce,
		    WPA_NONCE_LEN);
	/* IEEE 802.11i does not clear TimeoutCtr here, but this is more
	 * logical place than INITIALIZE since AUTHENTICATION2 can be
	 * re-entered on ReAuthenticationRequest without going through
	 * INITIALIZE. */
	sm->TimeoutCtr = 0;
}


SM_STATE(WPA_PTK, INITPMK)
{
	u8 msk[2 * PMK_LEN];
	size_t len = 2 * PMK_LEN;

	SM_ENTRY_MA(WPA_PTK, INITPMK, wpa_ptk);
#ifdef CONFIG_IEEE80211R
	sm->xxkey_len = 0;
#endif /* CONFIG_IEEE80211R */
	if (sm->pmksa) {
		wpa_printf(MSG_DEBUG, "WPA: PMK from PMKSA cache");
		os_memcpy(sm->PMK, sm->pmksa->pmk, PMK_LEN);
	} else if (wpa_auth_get_msk(sm->wpa_auth, sm->addr, msk, &len) == 0) {
		wpa_printf(MSG_DEBUG, "WPA: PMK from EAPOL state machine "
			   "(len=%lu)", (unsigned long) len);
		os_memcpy(sm->PMK, msk, PMK_LEN);
#ifdef CONFIG_IEEE80211R
		if (len >= 2 * PMK_LEN) {
			os_memcpy(sm->xxkey, msk + PMK_LEN, PMK_LEN);
			sm->xxkey_len = PMK_LEN;
		}
#endif /* CONFIG_IEEE80211R */
	} else {
		wpa_printf(MSG_DEBUG, "WPA: Could not get PMK");
	}

	sm->req_replay_counter_used = 0;
	/* IEEE 802.11i does not set keyRun to FALSE, but not doing this
	 * will break reauthentication since EAPOL state machines may not be
	 * get into AUTHENTICATING state that clears keyRun before WPA state
	 * machine enters AUTHENTICATION2 state and goes immediately to INITPMK
	 * state and takes PMK from the previously used AAA Key. This will
	 * eventually fail in 4-Way Handshake because Supplicant uses PMK
	 * derived from the new AAA Key. Setting keyRun = FALSE here seems to
	 * be good workaround for this issue. */
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_keyRun, 0);
}


SM_STATE(WPA_PTK, INITPSK)
{
	const u8 *psk;
	SM_ENTRY_MA(WPA_PTK, INITPSK, wpa_ptk);
	psk = wpa_auth_get_psk(sm->wpa_auth, sm->addr, sm->p2p_dev_addr, NULL);
	if (psk) {
		os_memcpy(sm->PMK, psk, PMK_LEN);
#ifdef CONFIG_IEEE80211R
		os_memcpy(sm->xxkey, psk, PMK_LEN);
		sm->xxkey_len = PMK_LEN;
#endif /* CONFIG_IEEE80211R */
	}
	sm->req_replay_counter_used = 0;
}


SM_STATE(WPA_PTK, PTKSTART)
{
	u8 buf[2 + RSN_SELECTOR_LEN + PMKID_LEN], *pmkid = NULL;
	size_t pmkid_len = 0;

	SM_ENTRY_MA(WPA_PTK, PTKSTART, wpa_ptk);
	sm->PTKRequest = FALSE;
	sm->TimeoutEvt = FALSE;

	sm->TimeoutCtr++;
	if (sm->TimeoutCtr > (int) dot11RSNAConfigPairwiseUpdateCount) {
		/* No point in sending the EAPOL-Key - we will disconnect
		 * immediately following this. */
		return;
	}

	wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
			"sending 1/4 msg of 4-Way Handshake");
	/*
	 * TODO: Could add PMKID even with WPA2-PSK, but only if there is only
	 * one possible PSK for this STA.
	 */
	if (sm->wpa == WPA_VERSION_WPA2 &&
	    wpa_key_mgmt_wpa_ieee8021x(sm->wpa_key_mgmt) &&
	    sm->wpa_key_mgmt != WPA_KEY_MGMT_OSEN) {
		pmkid = buf;
		pmkid_len = 2 + RSN_SELECTOR_LEN + PMKID_LEN;
		pmkid[0] = WLAN_EID_VENDOR_SPECIFIC;
		pmkid[1] = RSN_SELECTOR_LEN + PMKID_LEN;
		RSN_SELECTOR_PUT(&pmkid[2], RSN_KEY_DATA_PMKID);
		if (sm->pmksa)
			os_memcpy(&pmkid[2 + RSN_SELECTOR_LEN],
				  sm->pmksa->pmkid, PMKID_LEN);
		else {
			/*
			 * Calculate PMKID since no PMKSA cache entry was
			 * available with pre-calculated PMKID.
			 */
			rsn_pmkid(sm->PMK, PMK_LEN, sm->wpa_auth->addr,
				  sm->addr, &pmkid[2 + RSN_SELECTOR_LEN],
				  wpa_key_mgmt_sha256(sm->wpa_key_mgmt));
		}
	}
	wpa_send_eapol(sm->wpa_auth, sm,
		       WPA_KEY_INFO_ACK | WPA_KEY_INFO_KEY_TYPE, NULL,
		       sm->ANonce, pmkid, pmkid_len, 0, 0);
}


static int wpa_derive_ptk(struct wpa_state_machine *sm, const u8 *pmk,
			  struct wpa_ptk *ptk)
{
	size_t ptk_len = wpa_cipher_key_len(sm->pairwise) + 32;
#ifdef CONFIG_IEEE80211R
	if (wpa_key_mgmt_ft(sm->wpa_key_mgmt))
		return wpa_auth_derive_ptk_ft(sm, pmk, ptk, ptk_len);
#endif /* CONFIG_IEEE80211R */

	wpa_pmk_to_ptk(pmk, PMK_LEN, "Pairwise key expansion",
		       sm->wpa_auth->addr, sm->addr, sm->ANonce, sm->SNonce,
		       (u8 *) ptk, ptk_len,
		       wpa_key_mgmt_sha256(sm->wpa_key_mgmt));

	return 0;
}


SM_STATE(WPA_PTK, PTKCALCNEGOTIATING)
{
	struct wpa_ptk PTK;
	int ok = 0;
	const u8 *pmk = NULL;

	SM_ENTRY_MA(WPA_PTK, PTKCALCNEGOTIATING, wpa_ptk);
	sm->EAPOLKeyReceived = FALSE;
	sm->update_snonce = FALSE;

	/* WPA with IEEE 802.1X: use the derived PMK from EAP
	 * WPA-PSK: iterate through possible PSKs and select the one matching
	 * the packet */
	for (;;) {
		if (wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt)) {
			pmk = wpa_auth_get_psk(sm->wpa_auth, sm->addr,
					       sm->p2p_dev_addr, pmk);
			if (pmk == NULL)
				break;
		} else
			pmk = sm->PMK;

		wpa_derive_ptk(sm, pmk, &PTK);

		if (wpa_verify_key_mic(&PTK, sm->last_rx_eapol_key,
				       sm->last_rx_eapol_key_len) == 0) {
			ok = 1;
			break;
		}

		if (!wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt))
			break;
	}

	if (!ok) {
		wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
				"invalid MIC in msg 2/4 of 4-Way Handshake");
		return;
	}

#ifdef CONFIG_IEEE80211R
	if (sm->wpa == WPA_VERSION_WPA2 && wpa_key_mgmt_ft(sm->wpa_key_mgmt)) {
		/*
		 * Verify that PMKR1Name from EAPOL-Key message 2/4 matches
		 * with the value we derived.
		 */
		if (os_memcmp_const(sm->sup_pmk_r1_name, sm->pmk_r1_name,
				    WPA_PMK_NAME_LEN) != 0) {
			wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
					"PMKR1Name mismatch in FT 4-way "
					"handshake");
			wpa_hexdump(MSG_DEBUG, "FT: PMKR1Name from "
				    "Supplicant",
				    sm->sup_pmk_r1_name, WPA_PMK_NAME_LEN);
			wpa_hexdump(MSG_DEBUG, "FT: Derived PMKR1Name",
				    sm->pmk_r1_name, WPA_PMK_NAME_LEN);
			return;
		}
	}
#endif /* CONFIG_IEEE80211R */

	sm->pending_1_of_4_timeout = 0;
	eloop_cancel_timeout(wpa_send_eapol_timeout, sm->wpa_auth, sm);

	if (wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt)) {
		/* PSK may have changed from the previous choice, so update
		 * state machine data based on whatever PSK was selected here.
		 */
		os_memcpy(sm->PMK, pmk, PMK_LEN);
	}

	sm->MICVerified = TRUE;

	os_memcpy(&sm->PTK, &PTK, sizeof(PTK));
	sm->PTK_valid = TRUE;
}


SM_STATE(WPA_PTK, PTKCALCNEGOTIATING2)
{
	SM_ENTRY_MA(WPA_PTK, PTKCALCNEGOTIATING2, wpa_ptk);
	sm->TimeoutCtr = 0;
}


#ifdef CONFIG_IEEE80211W

static int ieee80211w_kde_len(struct wpa_state_machine *sm)
{
	if (sm->mgmt_frame_prot) {
		size_t len;
		len = wpa_cipher_key_len(sm->wpa_auth->conf.group_mgmt_cipher);
		return 2 + RSN_SELECTOR_LEN + WPA_IGTK_KDE_PREFIX_LEN + len;
	}

	return 0;
}


static u8 * ieee80211w_kde_add(struct wpa_state_machine *sm, u8 *pos)
{
	struct wpa_igtk_kde igtk;
	struct wpa_group *gsm = sm->group;
	u8 rsc[WPA_KEY_RSC_LEN];
	size_t len = wpa_cipher_key_len(sm->wpa_auth->conf.group_mgmt_cipher);

	if (!sm->mgmt_frame_prot)
		return pos;

	igtk.keyid[0] = gsm->GN_igtk;
	igtk.keyid[1] = 0;
	if (gsm->wpa_group_state != WPA_GROUP_SETKEYSDONE ||
	    wpa_auth_get_seqnum(sm->wpa_auth, NULL, gsm->GN_igtk, rsc) < 0)
		os_memset(igtk.pn, 0, sizeof(igtk.pn));
	else
		os_memcpy(igtk.pn, rsc, sizeof(igtk.pn));
	os_memcpy(igtk.igtk, gsm->IGTK[gsm->GN_igtk - 4], len);
	if (sm->wpa_auth->conf.disable_gtk) {
		/*
		 * Provide unique random IGTK to each STA to prevent use of
		 * IGTK in the BSS.
		 */
		if (random_get_bytes(igtk.igtk, len) < 0)
			return pos;
	}
	pos = wpa_add_kde(pos, RSN_KEY_DATA_IGTK,
			  (const u8 *) &igtk, WPA_IGTK_KDE_PREFIX_LEN + len,
			  NULL, 0);

	return pos;
}

#else /* CONFIG_IEEE80211W */

static int ieee80211w_kde_len(struct wpa_state_machine *sm)
{
	return 0;
}


static u8 * ieee80211w_kde_add(struct wpa_state_machine *sm, u8 *pos)
{
	return pos;
}

#endif /* CONFIG_IEEE80211W */


SM_STATE(WPA_PTK, PTKINITNEGOTIATING)
{
	u8 rsc[WPA_KEY_RSC_LEN], *_rsc, *gtk, *kde, *pos, dummy_gtk[32];
	size_t gtk_len, kde_len;
	struct wpa_group *gsm = sm->group;
	u8 *wpa_ie;
	int wpa_ie_len, secure, keyidx, encr = 0;

	SM_ENTRY_MA(WPA_PTK, PTKINITNEGOTIATING, wpa_ptk);
	sm->TimeoutEvt = FALSE;

	sm->TimeoutCtr++;
	if (sm->TimeoutCtr > (int) dot11RSNAConfigPairwiseUpdateCount) {
		/* No point in sending the EAPOL-Key - we will disconnect
		 * immediately following this. */
		return;
	}

	/* Send EAPOL(1, 1, 1, Pair, P, RSC, ANonce, MIC(PTK), RSNIE, [MDIE],
	   GTK[GN], IGTK, [FTIE], [TIE * 2])
	 */
	os_memset(rsc, 0, WPA_KEY_RSC_LEN);
	wpa_auth_get_seqnum(sm->wpa_auth, NULL, gsm->GN, rsc);
	/* If FT is used, wpa_auth->wpa_ie includes both RSNIE and MDIE */
	wpa_ie = sm->wpa_auth->wpa_ie;
	wpa_ie_len = sm->wpa_auth->wpa_ie_len;
	if (sm->wpa == WPA_VERSION_WPA &&
	    (sm->wpa_auth->conf.wpa & WPA_PROTO_RSN) &&
	    wpa_ie_len > wpa_ie[1] + 2 && wpa_ie[0] == WLAN_EID_RSN) {
		/* WPA-only STA, remove RSN IE */
		wpa_ie = wpa_ie + wpa_ie[1] + 2;
		wpa_ie_len = wpa_ie[1] + 2;
	}
	wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
			"sending 3/4 msg of 4-Way Handshake");
	if (sm->wpa == WPA_VERSION_WPA2) {
		/* WPA2 send GTK in the 4-way handshake */
		secure = 1;
		gtk = gsm->GTK[gsm->GN - 1];
		gtk_len = gsm->GTK_len;
		if (sm->wpa_auth->conf.disable_gtk) {
			/*
			 * Provide unique random GTK to each STA to prevent use
			 * of GTK in the BSS.
			 */
			if (random_get_bytes(dummy_gtk, gtk_len) < 0)
				return;
			gtk = dummy_gtk;
		}
		keyidx = gsm->GN;
		_rsc = rsc;
		encr = 1;
	} else {
		/* WPA does not include GTK in msg 3/4 */
		secure = 0;
		gtk = NULL;
		gtk_len = 0;
		keyidx = 0;
		_rsc = NULL;
		if (sm->rx_eapol_key_secure) {
			/*
			 * It looks like Windows 7 supplicant tries to use
			 * Secure bit in msg 2/4 after having reported Michael
			 * MIC failure and it then rejects the 4-way handshake
			 * if msg 3/4 does not set Secure bit. Work around this
			 * by setting the Secure bit here even in the case of
			 * WPA if the supplicant used it first.
			 */
			wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
					"STA used Secure bit in WPA msg 2/4 - "
					"set Secure for 3/4 as workaround");
			secure = 1;
		}
	}

	kde_len = wpa_ie_len + ieee80211w_kde_len(sm);
	if (gtk)
		kde_len += 2 + RSN_SELECTOR_LEN + 2 + gtk_len;
#ifdef CONFIG_IEEE80211R
	if (wpa_key_mgmt_ft(sm->wpa_key_mgmt)) {
		kde_len += 2 + PMKID_LEN; /* PMKR1Name into RSN IE */
		kde_len += 300; /* FTIE + 2 * TIE */
	}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_P2P
	if (WPA_GET_BE32(sm->ip_addr) > 0)
		kde_len += 2 + RSN_SELECTOR_LEN + 3 * 4;
#endif /* CONFIG_P2P */
	kde = os_malloc(kde_len);
	if (kde == NULL)
		return;

	pos = kde;
	os_memcpy(pos, wpa_ie, wpa_ie_len);
	pos += wpa_ie_len;
#ifdef CONFIG_IEEE80211R
	if (wpa_key_mgmt_ft(sm->wpa_key_mgmt)) {
		int res = wpa_insert_pmkid(kde, pos - kde, sm->pmk_r1_name);
		if (res < 0) {
			wpa_printf(MSG_ERROR, "FT: Failed to insert "
				   "PMKR1Name into RSN IE in EAPOL-Key data");
			os_free(kde);
			return;
		}
		pos += res;
	}
#endif /* CONFIG_IEEE80211R */
	if (gtk) {
		u8 hdr[2];
		hdr[0] = keyidx & 0x03;
		hdr[1] = 0;
		pos = wpa_add_kde(pos, RSN_KEY_DATA_GROUPKEY, hdr, 2,
				  gtk, gtk_len);
	}
	pos = ieee80211w_kde_add(sm, pos);

#ifdef CONFIG_IEEE80211R
	if (wpa_key_mgmt_ft(sm->wpa_key_mgmt)) {
		int res;
		struct wpa_auth_config *conf;

		conf = &sm->wpa_auth->conf;
		res = wpa_write_ftie(conf, conf->r0_key_holder,
				     conf->r0_key_holder_len,
				     NULL, NULL, pos, kde + kde_len - pos,
				     NULL, 0);
		if (res < 0) {
			wpa_printf(MSG_ERROR, "FT: Failed to insert FTIE "
				   "into EAPOL-Key Key Data");
			os_free(kde);
			return;
		}
		pos += res;

		/* TIE[ReassociationDeadline] (TU) */
		*pos++ = WLAN_EID_TIMEOUT_INTERVAL;
		*pos++ = 5;
		*pos++ = WLAN_TIMEOUT_REASSOC_DEADLINE;
		WPA_PUT_LE32(pos, conf->reassociation_deadline);
		pos += 4;

		/* TIE[KeyLifetime] (seconds) */
		*pos++ = WLAN_EID_TIMEOUT_INTERVAL;
		*pos++ = 5;
		*pos++ = WLAN_TIMEOUT_KEY_LIFETIME;
		WPA_PUT_LE32(pos, conf->r0_key_lifetime * 60);
		pos += 4;
	}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_P2P
	if (WPA_GET_BE32(sm->ip_addr) > 0) {
		u8 addr[3 * 4];
		os_memcpy(addr, sm->ip_addr, 4);
		os_memcpy(addr + 4, sm->wpa_auth->conf.ip_addr_mask, 4);
		os_memcpy(addr + 8, sm->wpa_auth->conf.ip_addr_go, 4);
		pos = wpa_add_kde(pos, WFA_KEY_DATA_IP_ADDR_ALLOC,
				  addr, sizeof(addr), NULL, 0);
	}
#endif /* CONFIG_P2P */

	wpa_send_eapol(sm->wpa_auth, sm,
		       (secure ? WPA_KEY_INFO_SECURE : 0) | WPA_KEY_INFO_MIC |
		       WPA_KEY_INFO_ACK | WPA_KEY_INFO_INSTALL |
		       WPA_KEY_INFO_KEY_TYPE,
		       _rsc, sm->ANonce, kde, pos - kde, keyidx, encr);
	os_free(kde);
}


SM_STATE(WPA_PTK, PTKINITDONE)
{
	SM_ENTRY_MA(WPA_PTK, PTKINITDONE, wpa_ptk);
	sm->EAPOLKeyReceived = FALSE;
	if (sm->Pair) {
		enum wpa_alg alg = wpa_cipher_to_alg(sm->pairwise);
		int klen = wpa_cipher_key_len(sm->pairwise);
		if (wpa_auth_set_key(sm->wpa_auth, 0, alg, sm->addr, 0,
				     sm->PTK.tk1, klen)) {
			wpa_sta_disconnect(sm->wpa_auth, sm->addr);
			return;
		}
		/* FIX: MLME-SetProtection.Request(TA, Tx_Rx) */
		sm->pairwise_set = TRUE;

		if (sm->wpa_auth->conf.wpa_ptk_rekey) {
			eloop_cancel_timeout(wpa_rekey_ptk, sm->wpa_auth, sm);
			eloop_register_timeout(sm->wpa_auth->conf.
					       wpa_ptk_rekey, 0, wpa_rekey_ptk,
					       sm->wpa_auth, sm);
		}

		if (wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt)) {
			wpa_auth_set_eapol(sm->wpa_auth, sm->addr,
					   WPA_EAPOL_authorized, 1);
		}
	}

	if (0 /* IBSS == TRUE */) {
		sm->keycount++;
		if (sm->keycount == 2) {
			wpa_auth_set_eapol(sm->wpa_auth, sm->addr,
					   WPA_EAPOL_portValid, 1);
		}
	} else {
		wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_portValid,
				   1);
	}
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_keyAvailable, 0);
	wpa_auth_set_eapol(sm->wpa_auth, sm->addr, WPA_EAPOL_keyDone, 1);
	if (sm->wpa == WPA_VERSION_WPA)
		sm->PInitAKeys = TRUE;
	else
		sm->has_GTK = TRUE;
	wpa_auth_vlogger(sm->wpa_auth, sm->addr, LOGGER_INFO,
			 "pairwise key handshake completed (%s)",
			 sm->wpa == WPA_VERSION_WPA ? "WPA" : "RSN");

#ifdef CONFIG_IEEE80211R
	wpa_ft_push_pmk_r1(sm->wpa_auth, sm->addr);
#endif /* CONFIG_IEEE80211R */
}


SM_STEP(WPA_PTK)
{
	struct wpa_authenticator *wpa_auth = sm->wpa_auth;

	if (sm->Init)
		SM_ENTER(WPA_PTK, INITIALIZE);
	else if (sm->Disconnect
		 /* || FIX: dot11RSNAConfigSALifetime timeout */) {
		wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG,
				"WPA_PTK: sm->Disconnect");
		SM_ENTER(WPA_PTK, DISCONNECT);
	}
	else if (sm->DeauthenticationRequest)
		SM_ENTER(WPA_PTK, DISCONNECTED);
	else if (sm->AuthenticationRequest)
		SM_ENTER(WPA_PTK, AUTHENTICATION);
	else if (sm->ReAuthenticationRequest)
		SM_ENTER(WPA_PTK, AUTHENTICATION2);
	else if (sm->PTKRequest)
		SM_ENTER(WPA_PTK, PTKSTART);
	else switch (sm->wpa_ptk_state) {
	case WPA_PTK_INITIALIZE:
		break;
	case WPA_PTK_DISCONNECT:
		SM_ENTER(WPA_PTK, DISCONNECTED);
		break;
	case WPA_PTK_DISCONNECTED:
		SM_ENTER(WPA_PTK, INITIALIZE);
		break;
	case WPA_PTK_AUTHENTICATION:
		SM_ENTER(WPA_PTK, AUTHENTICATION2);
		break;
	case WPA_PTK_AUTHENTICATION2:
		if (wpa_key_mgmt_wpa_ieee8021x(sm->wpa_key_mgmt) &&
		    wpa_auth_get_eapol(sm->wpa_auth, sm->addr,
				       WPA_EAPOL_keyRun) > 0)
			SM_ENTER(WPA_PTK, INITPMK);
		else if (wpa_key_mgmt_wpa_psk(sm->wpa_key_mgmt)
			 /* FIX: && 802.1X::keyRun */)
			SM_ENTER(WPA_PTK, INITPSK);
		break;
	case WPA_PTK_INITPMK:
		if (wpa_auth_get_eapol(sm->wpa_auth, sm->addr,
				       WPA_EAPOL_keyAvailable) > 0)
			SM_ENTER(WPA_PTK, PTKSTART);
		else {
			wpa_auth->dot11RSNA4WayHandshakeFailures++;
			wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_INFO,
					"INITPMK - keyAvailable = false");
			SM_ENTER(WPA_PTK, DISCONNECT);
		}
		break;
	case WPA_PTK_INITPSK:
		if (wpa_auth_get_psk(sm->wpa_auth, sm->addr, sm->p2p_dev_addr,
				     NULL))
			SM_ENTER(WPA_PTK, PTKSTART);
		else {
			wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_INFO,
					"no PSK configured for the STA");
			wpa_auth->dot11RSNA4WayHandshakeFailures++;
			SM_ENTER(WPA_PTK, DISCONNECT);
		}
		break;
	case WPA_PTK_PTKSTART:
		if (sm->EAPOLKeyReceived && !sm->EAPOLKeyRequest &&
		    sm->EAPOLKeyPairwise)
			SM_ENTER(WPA_PTK, PTKCALCNEGOTIATING);
		else if (sm->TimeoutCtr >
			 (int) dot11RSNAConfigPairwiseUpdateCount) {
			wpa_auth->dot11RSNA4WayHandshakeFailures++;
			wpa_auth_vlogger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
					 "PTKSTART: Retry limit %d reached",
					 dot11RSNAConfigPairwiseUpdateCount);
			SM_ENTER(WPA_PTK, DISCONNECT);
		} else if (sm->TimeoutEvt)
			SM_ENTER(WPA_PTK, PTKSTART);
		break;
	case WPA_PTK_PTKCALCNEGOTIATING:
		if (sm->MICVerified)
			SM_ENTER(WPA_PTK, PTKCALCNEGOTIATING2);
		else if (sm->EAPOLKeyReceived && !sm->EAPOLKeyRequest &&
			 sm->EAPOLKeyPairwise)
			SM_ENTER(WPA_PTK, PTKCALCNEGOTIATING);
		else if (sm->TimeoutEvt)
			SM_ENTER(WPA_PTK, PTKSTART);
		break;
	case WPA_PTK_PTKCALCNEGOTIATING2:
		SM_ENTER(WPA_PTK, PTKINITNEGOTIATING);
		break;
	case WPA_PTK_PTKINITNEGOTIATING:
		if (sm->update_snonce)
			SM_ENTER(WPA_PTK, PTKCALCNEGOTIATING);
		else if (sm->EAPOLKeyReceived && !sm->EAPOLKeyRequest &&
			 sm->EAPOLKeyPairwise && sm->MICVerified)
			SM_ENTER(WPA_PTK, PTKINITDONE);
		else if (sm->TimeoutCtr >
			 (int) dot11RSNAConfigPairwiseUpdateCount) {
			wpa_auth->dot11RSNA4WayHandshakeFailures++;
			wpa_auth_vlogger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
					 "PTKINITNEGOTIATING: Retry limit %d "
					 "reached",
					 dot11RSNAConfigPairwiseUpdateCount);
			SM_ENTER(WPA_PTK, DISCONNECT);
		} else if (sm->TimeoutEvt)
			SM_ENTER(WPA_PTK, PTKINITNEGOTIATING);
		break;
	case WPA_PTK_PTKINITDONE:
		break;
	}
}


SM_STATE(WPA_PTK_GROUP, IDLE)
{
	SM_ENTRY_MA(WPA_PTK_GROUP, IDLE, wpa_ptk_group);
	if (sm->Init) {
		/* Init flag is not cleared here, so avoid busy
		 * loop by claiming nothing changed. */
		sm->changed = FALSE;
	}
	sm->GTimeoutCtr = 0;
}


SM_STATE(WPA_PTK_GROUP, REKEYNEGOTIATING)
{
	u8 rsc[WPA_KEY_RSC_LEN];
	struct wpa_group *gsm = sm->group;
	u8 *kde, *pos, hdr[2];
	size_t kde_len;
	u8 *gtk, dummy_gtk[32];

	SM_ENTRY_MA(WPA_PTK_GROUP, REKEYNEGOTIATING, wpa_ptk_group);

	sm->GTimeoutCtr++;
	if (sm->GTimeoutCtr > (int) dot11RSNAConfigGroupUpdateCount) {
		/* No point in sending the EAPOL-Key - we will disconnect
		 * immediately following this. */
		return;
	}

	if (sm->wpa == WPA_VERSION_WPA)
		sm->PInitAKeys = FALSE;
	sm->TimeoutEvt = FALSE;
	/* Send EAPOL(1, 1, 1, !Pair, G, RSC, GNonce, MIC(PTK), GTK[GN]) */
	os_memset(rsc, 0, WPA_KEY_RSC_LEN);
	if (gsm->wpa_group_state == WPA_GROUP_SETKEYSDONE)
		wpa_auth_get_seqnum(sm->wpa_auth, NULL, gsm->GN, rsc);
	wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
			"sending 1/2 msg of Group Key Handshake");

	gtk = gsm->GTK[gsm->GN - 1];
	if (sm->wpa_auth->conf.disable_gtk) {
		/*
		 * Provide unique random GTK to each STA to prevent use
		 * of GTK in the BSS.
		 */
		if (random_get_bytes(dummy_gtk, gsm->GTK_len) < 0)
			return;
		gtk = dummy_gtk;
	}
	if (sm->wpa == WPA_VERSION_WPA2) {
		kde_len = 2 + RSN_SELECTOR_LEN + 2 + gsm->GTK_len +
			ieee80211w_kde_len(sm);
		kde = os_malloc(kde_len);
		if (kde == NULL)
			return;

		pos = kde;
		hdr[0] = gsm->GN & 0x03;
		hdr[1] = 0;
		pos = wpa_add_kde(pos, RSN_KEY_DATA_GROUPKEY, hdr, 2,
				  gtk, gsm->GTK_len);
		pos = ieee80211w_kde_add(sm, pos);
	} else {
		kde = gtk;
		pos = kde + gsm->GTK_len;
	}

	wpa_send_eapol(sm->wpa_auth, sm,
		       WPA_KEY_INFO_SECURE | WPA_KEY_INFO_MIC |
		       WPA_KEY_INFO_ACK |
		       (!sm->Pair ? WPA_KEY_INFO_INSTALL : 0),
		       rsc, gsm->GNonce, kde, pos - kde, gsm->GN, 1);
	if (sm->wpa == WPA_VERSION_WPA2)
		os_free(kde);
}


SM_STATE(WPA_PTK_GROUP, REKEYESTABLISHED)
{
	SM_ENTRY_MA(WPA_PTK_GROUP, REKEYESTABLISHED, wpa_ptk_group);
	sm->EAPOLKeyReceived = FALSE;
	if (sm->GUpdateStationKeys)
		sm->group->GKeyDoneStations--;
	sm->GUpdateStationKeys = FALSE;
	sm->GTimeoutCtr = 0;
	/* FIX: MLME.SetProtection.Request(TA, Tx_Rx) */
	wpa_auth_vlogger(sm->wpa_auth, sm->addr, LOGGER_INFO,
			 "group key handshake completed (%s)",
			 sm->wpa == WPA_VERSION_WPA ? "WPA" : "RSN");
	sm->has_GTK = TRUE;
}


SM_STATE(WPA_PTK_GROUP, KEYERROR)
{
	SM_ENTRY_MA(WPA_PTK_GROUP, KEYERROR, wpa_ptk_group);
	if (sm->GUpdateStationKeys)
		sm->group->GKeyDoneStations--;
	sm->GUpdateStationKeys = FALSE;
	sm->Disconnect = TRUE;
}


SM_STEP(WPA_PTK_GROUP)
{
	if (sm->Init || sm->PtkGroupInit) {
		SM_ENTER(WPA_PTK_GROUP, IDLE);
		sm->PtkGroupInit = FALSE;
	} else switch (sm->wpa_ptk_group_state) {
	case WPA_PTK_GROUP_IDLE:
		if (sm->GUpdateStationKeys ||
		    (sm->wpa == WPA_VERSION_WPA && sm->PInitAKeys))
			SM_ENTER(WPA_PTK_GROUP, REKEYNEGOTIATING);
		break;
	case WPA_PTK_GROUP_REKEYNEGOTIATING:
		if (sm->EAPOLKeyReceived && !sm->EAPOLKeyRequest &&
		    !sm->EAPOLKeyPairwise && sm->MICVerified)
			SM_ENTER(WPA_PTK_GROUP, REKEYESTABLISHED);
		else if (sm->GTimeoutCtr >
			 (int) dot11RSNAConfigGroupUpdateCount)
			SM_ENTER(WPA_PTK_GROUP, KEYERROR);
		else if (sm->TimeoutEvt)
			SM_ENTER(WPA_PTK_GROUP, REKEYNEGOTIATING);
		break;
	case WPA_PTK_GROUP_KEYERROR:
		SM_ENTER(WPA_PTK_GROUP, IDLE);
		break;
	case WPA_PTK_GROUP_REKEYESTABLISHED:
		SM_ENTER(WPA_PTK_GROUP, IDLE);
		break;
	}
}


static int wpa_gtk_update(struct wpa_authenticator *wpa_auth,
			  struct wpa_group *group)
{
	int ret = 0;

	os_memcpy(group->GNonce, group->Counter, WPA_NONCE_LEN);
	inc_byte_array(group->Counter, WPA_NONCE_LEN);
	if (wpa_gmk_to_gtk(group->GMK, "Group key expansion",
			   wpa_auth->addr, group->GNonce,
			   group->GTK[group->GN - 1], group->GTK_len) < 0)
		ret = -1;
	wpa_hexdump_key(MSG_DEBUG, "GTK",
			group->GTK[group->GN - 1], group->GTK_len);

#ifdef CONFIG_IEEE80211W
	if (wpa_auth->conf.ieee80211w != NO_MGMT_FRAME_PROTECTION) {
		size_t len;
		len = wpa_cipher_key_len(wpa_auth->conf.group_mgmt_cipher);
		os_memcpy(group->GNonce, group->Counter, WPA_NONCE_LEN);
		inc_byte_array(group->Counter, WPA_NONCE_LEN);
		if (wpa_gmk_to_gtk(group->GMK, "IGTK key expansion",
				   wpa_auth->addr, group->GNonce,
				   group->IGTK[group->GN_igtk - 4], len) < 0)
			ret = -1;
		wpa_hexdump_key(MSG_DEBUG, "IGTK",
				group->IGTK[group->GN_igtk - 4], len);
	}
#endif /* CONFIG_IEEE80211W */

	return ret;
}


static void wpa_group_gtk_init(struct wpa_authenticator *wpa_auth,
			       struct wpa_group *group)
{
	wpa_printf(MSG_DEBUG, "WPA: group state machine entering state "
		   "GTK_INIT (VLAN-ID %d)", group->vlan_id);
	group->changed = FALSE; /* GInit is not cleared here; avoid loop */
	group->wpa_group_state = WPA_GROUP_GTK_INIT;

	/* GTK[0..N] = 0 */
	os_memset(group->GTK, 0, sizeof(group->GTK));
	group->GN = 1;
	group->GM = 2;
#ifdef CONFIG_IEEE80211W
	group->GN_igtk = 4;
	group->GM_igtk = 5;
#endif /* CONFIG_IEEE80211W */
	/* GTK[GN] = CalcGTK() */
	wpa_gtk_update(wpa_auth, group);
}


static int wpa_group_update_sta(struct wpa_state_machine *sm, void *ctx)
{
	if (ctx != NULL && ctx != sm->group)
		return 0;

	if (sm->wpa_ptk_state != WPA_PTK_PTKINITDONE) {
		wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
				"Not in PTKINITDONE; skip Group Key update");
		sm->GUpdateStationKeys = FALSE;
		return 0;
	}
	if (sm->GUpdateStationKeys) {
		/*
		 * This should not really happen, so add a debug log entry.
		 * Since we clear the GKeyDoneStations before the loop, the
		 * station needs to be counted here anyway.
		 */
		wpa_auth_logger(sm->wpa_auth, sm->addr, LOGGER_DEBUG,
				"GUpdateStationKeys was already set when "
				"marking station for GTK rekeying");
	}

	/* Do not rekey GTK/IGTK when STA is in WNM-Sleep Mode */
	if (sm->is_wnmsleep)
		return 0;

	sm->group->GKeyDoneStations++;
	sm->GUpdateStationKeys = TRUE;

	wpa_sm_step(sm);
	return 0;
}


#ifdef CONFIG_WNM
/* update GTK when exiting WNM-Sleep Mode */
void wpa_wnmsleep_rekey_gtk(struct wpa_state_machine *sm)
{
	if (sm == NULL || sm->is_wnmsleep)
		return;

	wpa_group_update_sta(sm, NULL);
}


void wpa_set_wnmsleep(struct wpa_state_machine *sm, int flag)
{
	if (sm)
		sm->is_wnmsleep = !!flag;
}


int wpa_wnmsleep_gtk_subelem(struct wpa_state_machine *sm, u8 *pos)
{
	struct wpa_group *gsm = sm->group;
	u8 *start = pos;

	/*
	 * GTK subelement:
	 * Sub-elem ID[1] | Length[1] | Key Info[2] | Key Length[1] | RSC[8] |
	 * Key[5..32]
	 */
	*pos++ = WNM_SLEEP_SUBELEM_GTK;
	*pos++ = 11 + gsm->GTK_len;
	/* Key ID in B0-B1 of Key Info */
	WPA_PUT_LE16(pos, gsm->GN & 0x03);
	pos += 2;
	*pos++ = gsm->GTK_len;
	if (wpa_auth_get_seqnum(sm->wpa_auth, NULL, gsm->GN, pos) != 0)
		return 0;
	pos += 8;
	os_memcpy(pos, gsm->GTK[gsm->GN - 1], gsm->GTK_len);
	pos += gsm->GTK_len;

	wpa_printf(MSG_DEBUG, "WNM: GTK Key ID %u in WNM-Sleep Mode exit",
		   gsm->GN);
	wpa_hexdump_key(MSG_DEBUG, "WNM: GTK in WNM-Sleep Mode exit",
			gsm->GTK[gsm->GN - 1], gsm->GTK_len);

	return pos - start;
}


#ifdef CONFIG_IEEE80211W
int wpa_wnmsleep_igtk_subelem(struct wpa_state_machine *sm, u8 *pos)
{
	struct wpa_group *gsm = sm->group;
	u8 *start = pos;
	size_t len = wpa_cipher_key_len(sm->wpa_auth->conf.group_mgmt_cipher);

	/*
	 * IGTK subelement:
	 * Sub-elem ID[1] | Length[1] | KeyID[2] | PN[6] | Key[16]
	 */
	*pos++ = WNM_SLEEP_SUBELEM_IGTK;
	*pos++ = 2 + 6 + len;
	WPA_PUT_LE16(pos, gsm->GN_igtk);
	pos += 2;
	if (wpa_auth_get_seqnum(sm->wpa_auth, NULL, gsm->GN_igtk, pos) != 0)
		return 0;
	pos += 6;

	os_memcpy(pos, gsm->IGTK[gsm->GN_igtk - 4], len);
	pos += len;

	wpa_printf(MSG_DEBUG, "WNM: IGTK Key ID %u in WNM-Sleep Mode exit",
		   gsm->GN_igtk);
	wpa_hexdump_key(MSG_DEBUG, "WNM: IGTK in WNM-Sleep Mode exit",
			gsm->IGTK[gsm->GN_igtk - 4], len);

	return pos - start;
}
#endif /* CONFIG_IEEE80211W */
#endif /* CONFIG_WNM */


static void wpa_group_setkeys(struct wpa_authenticator *wpa_auth,
			      struct wpa_group *group)
{
	int tmp;

	wpa_printf(MSG_DEBUG, "WPA: group state machine entering state "
		   "SETKEYS (VLAN-ID %d)", group->vlan_id);
	group->changed = TRUE;
	group->wpa_group_state = WPA_GROUP_SETKEYS;
	group->GTKReKey = FALSE;
	tmp = group->GM;
	group->GM = group->GN;
	group->GN = tmp;
#ifdef CONFIG_IEEE80211W
	tmp = group->GM_igtk;
	group->GM_igtk = group->GN_igtk;
	group->GN_igtk = tmp;
#endif /* CONFIG_IEEE80211W */
	/* "GKeyDoneStations = GNoStations" is done in more robust way by
	 * counting the STAs that are marked with GUpdateStationKeys instead of
	 * including all STAs that could be in not-yet-completed state. */
	wpa_gtk_update(wpa_auth, group);

	if (group->GKeyDoneStations) {
		wpa_printf(MSG_DEBUG, "wpa_group_setkeys: Unexpected "
			   "GKeyDoneStations=%d when starting new GTK rekey",
			   group->GKeyDoneStations);
		group->GKeyDoneStations = 0;
	}
	wpa_auth_for_each_sta(wpa_auth, wpa_group_update_sta, group);
	wpa_printf(MSG_DEBUG, "wpa_group_setkeys: GKeyDoneStations=%d",
		   group->GKeyDoneStations);
}


static int wpa_group_config_group_keys(struct wpa_authenticator *wpa_auth,
				       struct wpa_group *group)
{
	int ret = 0;

	if (wpa_auth_set_key(wpa_auth, group->vlan_id,
			     wpa_cipher_to_alg(wpa_auth->conf.wpa_group),
			     broadcast_ether_addr, group->GN,
			     group->GTK[group->GN - 1], group->GTK_len) < 0)
		ret = -1;

#ifdef CONFIG_IEEE80211W
	if (wpa_auth->conf.ieee80211w != NO_MGMT_FRAME_PROTECTION) {
		enum wpa_alg alg;
		size_t len;

		alg = wpa_cipher_to_alg(wpa_auth->conf.group_mgmt_cipher);
		len = wpa_cipher_key_len(wpa_auth->conf.group_mgmt_cipher);

		if (ret == 0 &&
		    wpa_auth_set_key(wpa_auth, group->vlan_id, alg,
				     broadcast_ether_addr, group->GN_igtk,
				     group->IGTK[group->GN_igtk - 4], len) < 0)
			ret = -1;
	}
#endif /* CONFIG_IEEE80211W */

	return ret;
}


static int wpa_group_disconnect_cb(struct wpa_state_machine *sm, void *ctx)
{
	if (sm->group == ctx) {
		wpa_printf(MSG_DEBUG, "WPA: Mark STA " MACSTR
			   " for discconnection due to fatal failure",
			   MAC2STR(sm->addr));
		sm->Disconnect = TRUE;
	}

	return 0;
}


static void wpa_group_fatal_failure(struct wpa_authenticator *wpa_auth,
				    struct wpa_group *group)
{
	wpa_printf(MSG_DEBUG, "WPA: group state machine entering state FATAL_FAILURE");
	group->changed = TRUE;
	group->wpa_group_state = WPA_GROUP_FATAL_FAILURE;
	wpa_auth_for_each_sta(wpa_auth, wpa_group_disconnect_cb, group);
}


static int wpa_group_setkeysdone(struct wpa_authenticator *wpa_auth,
				 struct wpa_group *group)
{
	wpa_printf(MSG_DEBUG, "WPA: group state machine entering state "
		   "SETKEYSDONE (VLAN-ID %d)", group->vlan_id);
	group->changed = TRUE;
	group->wpa_group_state = WPA_GROUP_SETKEYSDONE;

	if (wpa_group_config_group_keys(wpa_auth, group) < 0) {
		wpa_group_fatal_failure(wpa_auth, group);
		return -1;
	}

	return 0;
}


static void wpa_group_sm_step(struct wpa_authenticator *wpa_auth,
			      struct wpa_group *group)
{
	if (group->GInit) {
		wpa_group_gtk_init(wpa_auth, group);
	} else if (group->wpa_group_state == WPA_GROUP_FATAL_FAILURE) {
		/* Do not allow group operations */
	} else if (group->wpa_group_state == WPA_GROUP_GTK_INIT &&
		   group->GTKAuthenticator) {
		wpa_group_setkeysdone(wpa_auth, group);
	} else if (group->wpa_group_state == WPA_GROUP_SETKEYSDONE &&
		   group->GTKReKey) {
		wpa_group_setkeys(wpa_auth, group);
	} else if (group->wpa_group_state == WPA_GROUP_SETKEYS) {
		if (group->GKeyDoneStations == 0)
			wpa_group_setkeysdone(wpa_auth, group);
		else if (group->GTKReKey)
			wpa_group_setkeys(wpa_auth, group);
	}
}


static int wpa_sm_step(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return 0;

	if (sm->in_step_loop) {
		/* This should not happen, but if it does, make sure we do not
		 * end up freeing the state machine too early by exiting the
		 * recursive call. */
		wpa_printf(MSG_ERROR, "WPA: wpa_sm_step() called recursively");
		return 0;
	}

	sm->in_step_loop = 1;
	do {
		if (sm->pending_deinit)
			break;

		sm->changed = FALSE;
		sm->wpa_auth->group->changed = FALSE;

		SM_STEP_RUN(WPA_PTK);
		if (sm->pending_deinit)
			break;
		SM_STEP_RUN(WPA_PTK_GROUP);
		if (sm->pending_deinit)
			break;
		wpa_group_sm_step(sm->wpa_auth, sm->group);
	} while (sm->changed || sm->wpa_auth->group->changed);
	sm->in_step_loop = 0;

	if (sm->pending_deinit) {
		wpa_printf(MSG_DEBUG, "WPA: Completing pending STA state "
			   "machine deinit for " MACSTR, MAC2STR(sm->addr));
		wpa_free_sta_sm(sm);
		return 1;
	}
	return 0;
}


static void wpa_sm_call_step(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_state_machine *sm = eloop_ctx;
	wpa_sm_step(sm);
}


void wpa_auth_sm_notify(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return;
	eloop_register_timeout(0, 0, wpa_sm_call_step, sm, NULL);
}


void wpa_gtk_rekey(struct wpa_authenticator *wpa_auth)
{
	int tmp, i;
	struct wpa_group *group;

	if (wpa_auth == NULL)
		return;

	group = wpa_auth->group;

	for (i = 0; i < 2; i++) {
		tmp = group->GM;
		group->GM = group->GN;
		group->GN = tmp;
#ifdef CONFIG_IEEE80211W
		tmp = group->GM_igtk;
		group->GM_igtk = group->GN_igtk;
		group->GN_igtk = tmp;
#endif /* CONFIG_IEEE80211W */
		wpa_gtk_update(wpa_auth, group);
		wpa_group_config_group_keys(wpa_auth, group);
	}
}


static const char * wpa_bool_txt(int bool)
{
	return bool ? "TRUE" : "FALSE";
}


#define RSN_SUITE "%02x-%02x-%02x-%d"
#define RSN_SUITE_ARG(s) \
((s) >> 24) & 0xff, ((s) >> 16) & 0xff, ((s) >> 8) & 0xff, (s) & 0xff

int wpa_get_mib(struct wpa_authenticator *wpa_auth, char *buf, size_t buflen)
{
	int len = 0, ret;
	char pmkid_txt[PMKID_LEN * 2 + 1];
#ifdef CONFIG_RSN_PREAUTH
	const int preauth = 1;
#else /* CONFIG_RSN_PREAUTH */
	const int preauth = 0;
#endif /* CONFIG_RSN_PREAUTH */

	if (wpa_auth == NULL)
		return len;

	ret = os_snprintf(buf + len, buflen - len,
			  "dot11RSNAOptionImplemented=TRUE\n"
			  "dot11RSNAPreauthenticationImplemented=%s\n"
			  "dot11RSNAEnabled=%s\n"
			  "dot11RSNAPreauthenticationEnabled=%s\n",
			  wpa_bool_txt(preauth),
			  wpa_bool_txt(wpa_auth->conf.wpa & WPA_PROTO_RSN),
			  wpa_bool_txt(wpa_auth->conf.rsn_preauth));
	if (ret < 0 || (size_t) ret >= buflen - len)
		return len;
	len += ret;

	wpa_snprintf_hex(pmkid_txt, sizeof(pmkid_txt),
			 wpa_auth->dot11RSNAPMKIDUsed, PMKID_LEN);

	ret = os_snprintf(
		buf + len, buflen - len,
		"dot11RSNAConfigVersion=%u\n"
		"dot11RSNAConfigPairwiseKeysSupported=9999\n"
		/* FIX: dot11RSNAConfigGroupCipher */
		/* FIX: dot11RSNAConfigGroupRekeyMethod */
		/* FIX: dot11RSNAConfigGroupRekeyTime */
		/* FIX: dot11RSNAConfigGroupRekeyPackets */
		"dot11RSNAConfigGroupRekeyStrict=%u\n"
		"dot11RSNAConfigGroupUpdateCount=%u\n"
		"dot11RSNAConfigPairwiseUpdateCount=%u\n"
		"dot11RSNAConfigGroupCipherSize=%u\n"
		"dot11RSNAConfigPMKLifetime=%u\n"
		"dot11RSNAConfigPMKReauthThreshold=%u\n"
		"dot11RSNAConfigNumberOfPTKSAReplayCounters=0\n"
		"dot11RSNAConfigSATimeout=%u\n"
		"dot11RSNAAuthenticationSuiteSelected=" RSN_SUITE "\n"
		"dot11RSNAPairwiseCipherSelected=" RSN_SUITE "\n"
		"dot11RSNAGroupCipherSelected=" RSN_SUITE "\n"
		"dot11RSNAPMKIDUsed=%s\n"
		"dot11RSNAAuthenticationSuiteRequested=" RSN_SUITE "\n"
		"dot11RSNAPairwiseCipherRequested=" RSN_SUITE "\n"
		"dot11RSNAGroupCipherRequested=" RSN_SUITE "\n"
		"dot11RSNATKIPCounterMeasuresInvoked=%u\n"
		"dot11RSNA4WayHandshakeFailures=%u\n"
		"dot11RSNAConfigNumberOfGTKSAReplayCounters=0\n",
		RSN_VERSION,
		!!wpa_auth->conf.wpa_strict_rekey,
		dot11RSNAConfigGroupUpdateCount,
		dot11RSNAConfigPairwiseUpdateCount,
		wpa_cipher_key_len(wpa_auth->conf.wpa_group) * 8,
		dot11RSNAConfigPMKLifetime,
		dot11RSNAConfigPMKReauthThreshold,
		dot11RSNAConfigSATimeout,
		RSN_SUITE_ARG(wpa_auth->dot11RSNAAuthenticationSuiteSelected),
		RSN_SUITE_ARG(wpa_auth->dot11RSNAPairwiseCipherSelected),
		RSN_SUITE_ARG(wpa_auth->dot11RSNAGroupCipherSelected),
		pmkid_txt,
		RSN_SUITE_ARG(wpa_auth->dot11RSNAAuthenticationSuiteRequested),
		RSN_SUITE_ARG(wpa_auth->dot11RSNAPairwiseCipherRequested),
		RSN_SUITE_ARG(wpa_auth->dot11RSNAGroupCipherRequested),
		wpa_auth->dot11RSNATKIPCounterMeasuresInvoked,
		wpa_auth->dot11RSNA4WayHandshakeFailures);
	if (ret < 0 || (size_t) ret >= buflen - len)
		return len;
	len += ret;

	/* TODO: dot11RSNAConfigPairwiseCiphersTable */
	/* TODO: dot11RSNAConfigAuthenticationSuitesTable */

	/* Private MIB */
	ret = os_snprintf(buf + len, buflen - len, "hostapdWPAGroupState=%d\n",
			  wpa_auth->group->wpa_group_state);
	if (ret < 0 || (size_t) ret >= buflen - len)
		return len;
	len += ret;

	return len;
}


int wpa_get_mib_sta(struct wpa_state_machine *sm, char *buf, size_t buflen)
{
	int len = 0, ret;
	u32 pairwise = 0;

	if (sm == NULL)
		return 0;

	/* TODO: FF-FF-FF-FF-FF-FF entry for broadcast/multicast stats */

	/* dot11RSNAStatsEntry */

	pairwise = wpa_cipher_to_suite(sm->wpa == WPA_VERSION_WPA2 ?
				       WPA_PROTO_RSN : WPA_PROTO_WPA,
				       sm->pairwise);
	if (pairwise == 0)
		return 0;

	ret = os_snprintf(
		buf + len, buflen - len,
		/* TODO: dot11RSNAStatsIndex */
		"dot11RSNAStatsSTAAddress=" MACSTR "\n"
		"dot11RSNAStatsVersion=1\n"
		"dot11RSNAStatsSelectedPairwiseCipher=" RSN_SUITE "\n"
		/* TODO: dot11RSNAStatsTKIPICVErrors */
		"dot11RSNAStatsTKIPLocalMICFailures=%u\n"
		"dot11RSNAStatsTKIPRemoteMICFailures=%u\n"
		/* TODO: dot11RSNAStatsCCMPReplays */
		/* TODO: dot11RSNAStatsCCMPDecryptErrors */
		/* TODO: dot11RSNAStatsTKIPReplays */,
		MAC2STR(sm->addr),
		RSN_SUITE_ARG(pairwise),
		sm->dot11RSNAStatsTKIPLocalMICFailures,
		sm->dot11RSNAStatsTKIPRemoteMICFailures);
	if (ret < 0 || (size_t) ret >= buflen - len)
		return len;
	len += ret;

	/* Private MIB */
	ret = os_snprintf(buf + len, buflen - len,
			  "hostapdWPAPTKState=%d\n"
			  "hostapdWPAPTKGroupState=%d\n",
			  sm->wpa_ptk_state,
			  sm->wpa_ptk_group_state);
	if (ret < 0 || (size_t) ret >= buflen - len)
		return len;
	len += ret;

	return len;
}


void wpa_auth_countermeasures_start(struct wpa_authenticator *wpa_auth)
{
	if (wpa_auth)
		wpa_auth->dot11RSNATKIPCounterMeasuresInvoked++;
}


int wpa_auth_pairwise_set(struct wpa_state_machine *sm)
{
	return sm && sm->pairwise_set;
}


int wpa_auth_get_pairwise(struct wpa_state_machine *sm)
{
	return sm->pairwise;
}


int wpa_auth_sta_key_mgmt(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return -1;
	return sm->wpa_key_mgmt;
}


int wpa_auth_sta_wpa_version(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return 0;
	return sm->wpa;
}


int wpa_auth_sta_clear_pmksa(struct wpa_state_machine *sm,
			     struct rsn_pmksa_cache_entry *entry)
{
	if (sm == NULL || sm->pmksa != entry)
		return -1;
	sm->pmksa = NULL;
	return 0;
}


struct rsn_pmksa_cache_entry *
wpa_auth_sta_get_pmksa(struct wpa_state_machine *sm)
{
	return sm ? sm->pmksa : NULL;
}


void wpa_auth_sta_local_mic_failure_report(struct wpa_state_machine *sm)
{
	if (sm)
		sm->dot11RSNAStatsTKIPLocalMICFailures++;
}


const u8 * wpa_auth_get_wpa_ie(struct wpa_authenticator *wpa_auth, size_t *len)
{
	if (wpa_auth == NULL)
		return NULL;
	*len = wpa_auth->wpa_ie_len;
	return wpa_auth->wpa_ie;
}


int wpa_auth_pmksa_add(struct wpa_state_machine *sm, const u8 *pmk,
		       int session_timeout, struct eapol_state_machine *eapol)
{
	if (sm == NULL || sm->wpa != WPA_VERSION_WPA2 ||
	    sm->wpa_auth->conf.disable_pmksa_caching)
		return -1;

	if (pmksa_cache_auth_add(sm->wpa_auth->pmksa, pmk, PMK_LEN,
				 sm->wpa_auth->addr, sm->addr, session_timeout,
				 eapol, sm->wpa_key_mgmt))
		return 0;

	return -1;
}


int wpa_auth_pmksa_add_preauth(struct wpa_authenticator *wpa_auth,
			       const u8 *pmk, size_t len, const u8 *sta_addr,
			       int session_timeout,
			       struct eapol_state_machine *eapol)
{
	if (wpa_auth == NULL)
		return -1;

	if (pmksa_cache_auth_add(wpa_auth->pmksa, pmk, len, wpa_auth->addr,
				 sta_addr, session_timeout, eapol,
				 WPA_KEY_MGMT_IEEE8021X))
		return 0;

	return -1;
}


void wpa_auth_pmksa_remove(struct wpa_authenticator *wpa_auth,
			   const u8 *sta_addr)
{
	struct rsn_pmksa_cache_entry *pmksa;

	if (wpa_auth == NULL || wpa_auth->pmksa == NULL)
		return;
	pmksa = pmksa_cache_auth_get(wpa_auth->pmksa, sta_addr, NULL);
	if (pmksa) {
		wpa_printf(MSG_DEBUG, "WPA: Remove PMKSA cache entry for "
			   MACSTR " based on request", MAC2STR(sta_addr));
		pmksa_cache_free_entry(wpa_auth->pmksa, pmksa);
	}
}


static struct wpa_group *
wpa_auth_add_group(struct wpa_authenticator *wpa_auth, int vlan_id)
{
	struct wpa_group *group;

	if (wpa_auth == NULL || wpa_auth->group == NULL)
		return NULL;

	wpa_printf(MSG_DEBUG, "WPA: Add group state machine for VLAN-ID %d",
		   vlan_id);
	group = wpa_group_init(wpa_auth, vlan_id, 0);
	if (group == NULL)
		return NULL;

	group->next = wpa_auth->group->next;
	wpa_auth->group->next = group;

	return group;
}


int wpa_auth_sta_set_vlan(struct wpa_state_machine *sm, int vlan_id)
{
	struct wpa_group *group;

	if (sm == NULL || sm->wpa_auth == NULL)
		return 0;

	group = sm->wpa_auth->group;
	while (group) {
		if (group->vlan_id == vlan_id)
			break;
		group = group->next;
	}

	if (group == NULL) {
		group = wpa_auth_add_group(sm->wpa_auth, vlan_id);
		if (group == NULL)
			return -1;
	}

	if (sm->group == group)
		return 0;

	if (group->wpa_group_state == WPA_GROUP_FATAL_FAILURE)
		return -1;

	wpa_printf(MSG_DEBUG, "WPA: Moving STA " MACSTR " to use group state "
		   "machine for VLAN ID %d", MAC2STR(sm->addr), vlan_id);

	sm->group = group;
	return 0;
}


void wpa_auth_eapol_key_tx_status(struct wpa_authenticator *wpa_auth,
				  struct wpa_state_machine *sm, int ack)
{
	if (wpa_auth == NULL || sm == NULL)
		return;
	wpa_printf(MSG_DEBUG, "WPA: EAPOL-Key TX status for STA " MACSTR
		   " ack=%d", MAC2STR(sm->addr), ack);
	if (sm->pending_1_of_4_timeout && ack) {
		/*
		 * Some deployed supplicant implementations update their SNonce
		 * for each EAPOL-Key 2/4 message even within the same 4-way
		 * handshake and then fail to use the first SNonce when
		 * deriving the PTK. This results in unsuccessful 4-way
		 * handshake whenever the relatively short initial timeout is
		 * reached and EAPOL-Key 1/4 is retransmitted. Try to work
		 * around this by increasing the timeout now that we know that
		 * the station has received the frame.
		 */
		int timeout_ms = eapol_key_timeout_subseq;
		wpa_printf(MSG_DEBUG, "WPA: Increase initial EAPOL-Key 1/4 "
			   "timeout by %u ms because of acknowledged frame",
			   timeout_ms);
		eloop_cancel_timeout(wpa_send_eapol_timeout, wpa_auth, sm);
		eloop_register_timeout(timeout_ms / 1000,
				       (timeout_ms % 1000) * 1000,
				       wpa_send_eapol_timeout, wpa_auth, sm);
	}
}


int wpa_auth_uses_sae(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return 0;
	return wpa_key_mgmt_sae(sm->wpa_key_mgmt);
}


int wpa_auth_uses_ft_sae(struct wpa_state_machine *sm)
{
	if (sm == NULL)
		return 0;
	return sm->wpa_key_mgmt == WPA_KEY_MGMT_FT_SAE;
}


#ifdef CONFIG_P2P
int wpa_auth_get_ip_addr(struct wpa_state_machine *sm, u8 *addr)
{
	if (sm == NULL || WPA_GET_BE32(sm->ip_addr) == 0)
		return -1;
	os_memcpy(addr, sm->ip_addr, 4);
	return 0;
}
#endif /* CONFIG_P2P */
