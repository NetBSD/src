/*
 * wpa_supplicant - Internal definitions
 * Copyright (c) 2003-2010, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef WPA_SUPPLICANT_I_H
#define WPA_SUPPLICANT_I_H

#include "utils/list.h"
#include "common/defs.h"
#include "config_ssid.h"

extern const char *wpa_supplicant_version;
extern const char *wpa_supplicant_license;
#ifndef CONFIG_NO_STDOUT_DEBUG
extern const char *wpa_supplicant_full_license1;
extern const char *wpa_supplicant_full_license2;
extern const char *wpa_supplicant_full_license3;
extern const char *wpa_supplicant_full_license4;
extern const char *wpa_supplicant_full_license5;
#endif /* CONFIG_NO_STDOUT_DEBUG */

struct wpa_sm;
struct wpa_supplicant;
struct ibss_rsn;
struct scan_info;
struct wpa_bss;
struct wpa_scan_results;
struct hostapd_hw_modes;

/*
 * Forward declarations of private structures used within the ctrl_iface
 * backends. Other parts of wpa_supplicant do not have access to data stored in
 * these structures.
 */
struct ctrl_iface_priv;
struct ctrl_iface_global_priv;
struct wpas_dbus_priv;

/**
 * struct wpa_interface - Parameters for wpa_supplicant_add_iface()
 */
struct wpa_interface {
	/**
	 * confname - Configuration name (file or profile) name
	 *
	 * This can also be %NULL when a configuration file is not used. In
	 * that case, ctrl_interface must be set to allow the interface to be
	 * configured.
	 */
	const char *confname;

	/**
	 * ctrl_interface - Control interface parameter
	 *
	 * If a configuration file is not used, this variable can be used to
	 * set the ctrl_interface parameter that would have otherwise been read
	 * from the configuration file. If both confname and ctrl_interface are
	 * set, ctrl_interface is used to override the value from configuration
	 * file.
	 */
	const char *ctrl_interface;

	/**
	 * driver - Driver interface name, or %NULL to use the default driver
	 */
	const char *driver;

	/**
	 * driver_param - Driver interface parameters
	 *
	 * If a configuration file is not used, this variable can be used to
	 * set the driver_param parameters that would have otherwise been read
	 * from the configuration file. If both confname and driver_param are
	 * set, driver_param is used to override the value from configuration
	 * file.
	 */
	const char *driver_param;

	/**
	 * ifname - Interface name
	 */
	const char *ifname;

	/**
	 * bridge_ifname - Optional bridge interface name
	 *
	 * If the driver interface (ifname) is included in a Linux bridge
	 * device, the bridge interface may need to be used for receiving EAPOL
	 * frames. This can be enabled by setting this variable to enable
	 * receiving of EAPOL frames from an additional interface.
	 */
	const char *bridge_ifname;
};

/**
 * struct wpa_params - Parameters for wpa_supplicant_init()
 */
struct wpa_params {
	/**
	 * daemonize - Run %wpa_supplicant in the background
	 */
	int daemonize;

	/**
	 * wait_for_monitor - Wait for a monitor program before starting
	 */
	int wait_for_monitor;

	/**
	 * pid_file - Path to a PID (process ID) file
	 *
	 * If this and daemonize are set, process ID of the background process
	 * will be written to the specified file.
	 */
	char *pid_file;

	/**
	 * wpa_debug_level - Debugging verbosity level (e.g., MSG_INFO)
	 */
	int wpa_debug_level;

	/**
	 * wpa_debug_show_keys - Whether keying material is included in debug
	 *
	 * This parameter can be used to allow keying material to be included
	 * in debug messages. This is a security risk and this option should
	 * not be enabled in normal configuration. If needed during
	 * development or while troubleshooting, this option can provide more
	 * details for figuring out what is happening.
	 */
	int wpa_debug_show_keys;

	/**
	 * wpa_debug_timestamp - Whether to include timestamp in debug messages
	 */
	int wpa_debug_timestamp;

	/**
	 * ctrl_interface - Global ctrl_iface path/parameter
	 */
	char *ctrl_interface;

	/**
	 * dbus_ctrl_interface - Enable the DBus control interface
	 */
	int dbus_ctrl_interface;

	/**
	 * wpa_debug_file_path - Path of debug file or %NULL to use stdout
	 */
	const char *wpa_debug_file_path;

	/**
	 * wpa_debug_syslog - Enable log output through syslog
	 */
	int wpa_debug_syslog;

	/**
	 * override_driver - Optional driver parameter override
	 *
	 * This parameter can be used to override the driver parameter in
	 * dynamic interface addition to force a specific driver wrapper to be
	 * used instead.
	 */
	char *override_driver;

	/**
	 * override_ctrl_interface - Optional ctrl_interface override
	 *
	 * This parameter can be used to override the ctrl_interface parameter
	 * in dynamic interface addition to force a control interface to be
	 * created.
	 */
	char *override_ctrl_interface;

	/**
	 * entropy_file - Optional entropy file
	 *
	 * This parameter can be used to configure wpa_supplicant to maintain
	 * its internal entropy store over restarts.
	 */
	char *entropy_file;
};

struct p2p_srv_bonjour {
	struct dl_list list;
	struct wpabuf *query;
	struct wpabuf *resp;
};

struct p2p_srv_upnp {
	struct dl_list list;
	u8 version;
	char *service;
};

/**
 * struct wpa_global - Internal, global data for all %wpa_supplicant interfaces
 *
 * This structure is initialized by calling wpa_supplicant_init() when starting
 * %wpa_supplicant.
 */
struct wpa_global {
	struct wpa_supplicant *ifaces;
	struct wpa_params params;
	struct ctrl_iface_global_priv *ctrl_iface;
	struct wpas_dbus_priv *dbus;
	void **drv_priv;
	size_t drv_count;
	struct os_time suspend_time;
	struct p2p_data *p2p;
	struct wpa_supplicant *p2p_group_formation;
	u8 p2p_dev_addr[ETH_ALEN];
	struct dl_list p2p_srv_bonjour; /* struct p2p_srv_bonjour */
	struct dl_list p2p_srv_upnp; /* struct p2p_srv_upnp */
	int p2p_disabled;
	int cross_connection;
};


enum offchannel_send_action_result {
	OFFCHANNEL_SEND_ACTION_SUCCESS /* Frame was send and acknowledged */,
	OFFCHANNEL_SEND_ACTION_NO_ACK /* Frame was sent, but not acknowledged
				       */,
	OFFCHANNEL_SEND_ACTION_FAILED /* Frame was not sent due to a failure */
};

/**
 * struct wpa_supplicant - Internal data for wpa_supplicant interface
 *
 * This structure contains the internal data for core wpa_supplicant code. This
 * should be only used directly from the core code. However, a pointer to this
 * data is used from other files as an arbitrary context pointer in calls to
 * core functions.
 */
struct wpa_supplicant {
	struct wpa_global *global;
	struct wpa_supplicant *parent;
	struct wpa_supplicant *next;
	struct l2_packet_data *l2;
	struct l2_packet_data *l2_br;
	unsigned char own_addr[ETH_ALEN];
	char ifname[100];
#ifdef CONFIG_CTRL_IFACE_DBUS
	char *dbus_path;
#endif /* CONFIG_CTRL_IFACE_DBUS */
#ifdef CONFIG_CTRL_IFACE_DBUS_NEW
	char *dbus_new_path;
	char *dbus_groupobj_path;
#endif /* CONFIG_CTRL_IFACE_DBUS_NEW */
	char bridge_ifname[16];

	char *confname;
	struct wpa_config *conf;
	int countermeasures;
	os_time_t last_michael_mic_error;
	u8 bssid[ETH_ALEN];
	u8 pending_bssid[ETH_ALEN]; /* If wpa_state == WPA_ASSOCIATING, this
				     * field contains the target BSSID. */
	int reassociate; /* reassociation requested */
	int disconnected; /* all connections disabled; i.e., do no reassociate
			   * before this has been cleared */
	struct wpa_ssid *current_ssid;
	struct wpa_bss *current_bss;
	int ap_ies_from_associnfo;
	unsigned int assoc_freq;

	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int wpa_proto;
	int mgmt_group_cipher;

	void *drv_priv; /* private data used by driver_ops */
	void *global_drv_priv;

	struct wpa_ssid *prev_scan_ssid; /* previously scanned SSID;
					  * NULL = not yet initialized (start
					  * with wildcard SSID)
					  * WILDCARD_SSID_SCAN = wildcard
					  * SSID was used in the previous scan
					  */
#define WILDCARD_SSID_SCAN ((struct wpa_ssid *) 1)

	struct wpa_ssid *prev_sched_ssid; /* last SSID used in sched scan */
	int sched_scan_timeout;
	int sched_scan_interval;
	int first_sched_scan;
	int sched_scan_timed_out;

	void (*scan_res_handler)(struct wpa_supplicant *wpa_s,
				 struct wpa_scan_results *scan_res);
	struct dl_list bss; /* struct wpa_bss::list */
	struct dl_list bss_id; /* struct wpa_bss::list_id */
	size_t num_bss;
	unsigned int bss_update_idx;
	unsigned int bss_next_id;

	struct wpa_driver_ops *driver;
	int interface_removed; /* whether the network interface has been
				* removed */
	struct wpa_sm *wpa;
	struct eapol_sm *eapol;

	struct ctrl_iface_priv *ctrl_iface;

	enum wpa_states wpa_state;
	int scanning;
	int sched_scanning;
	int new_connection;
	int reassociated_connection;

	int eapol_received; /* number of EAPOL packets received after the
			     * previous association event */

	struct scard_data *scard;

	unsigned char last_eapol_src[ETH_ALEN];

	int keys_cleared;

	struct wpa_blacklist *blacklist;

	int scan_req; /* manual scan request; this forces a scan even if there
		       * are no enabled networks in the configuration */
	int scan_runs; /* number of scan runs since WPS was started */
	int *next_scan_freqs;
	int scan_interval; /* time in sec between scans to find suitable AP */

	unsigned int drv_flags;
	int max_scan_ssids;
	int max_sched_scan_ssids;
	int sched_scan_supported;
	unsigned int max_match_sets;
	unsigned int max_remain_on_chan;
	unsigned int max_stations;

	int pending_mic_error_report;
	int pending_mic_error_pairwise;
	int mic_errors_seen; /* Michael MIC errors with the current PTK */

	struct wps_context *wps;
	int wps_success; /* WPS success event received */
	struct wps_er *wps_er;
	int blacklist_cleared;

	struct wpabuf *pending_eapol_rx;
	struct os_time pending_eapol_rx_time;
	u8 pending_eapol_rx_src[ETH_ALEN];

	struct ibss_rsn *ibss_rsn;

	int set_sta_uapsd;
	int sta_uapsd;
	int set_ap_uapsd;
	int ap_uapsd;

#ifdef CONFIG_SME
	struct {
		u8 ssid[32];
		size_t ssid_len;
		int freq;
		u8 assoc_req_ie[200];
		size_t assoc_req_ie_len;
		int mfp;
		int ft_used;
		u8 mobility_domain[2];
		u8 *ft_ies;
		size_t ft_ies_len;
		u8 prev_bssid[ETH_ALEN];
		int prev_bssid_set;
		int auth_alg;
		int proto;

		int sa_query_count; /* number of pending SA Query requests;
				     * 0 = no SA Query in progress */
		int sa_query_timed_out;
		u8 *sa_query_trans_id; /* buffer of WLAN_SA_QUERY_TR_ID_LEN *
					* sa_query_count octets of pending
					* SA Query transaction identifiers */
		struct os_time sa_query_start;
	} sme;
#endif /* CONFIG_SME */

#ifdef CONFIG_AP
	struct hostapd_iface *ap_iface;
	void (*ap_configured_cb)(void *ctx, void *data);
	void *ap_configured_cb_ctx;
	void *ap_configured_cb_data;
#endif /* CONFIG_AP */

	unsigned int off_channel_freq;
	struct wpabuf *pending_action_tx;
	u8 pending_action_src[ETH_ALEN];
	u8 pending_action_dst[ETH_ALEN];
	u8 pending_action_bssid[ETH_ALEN];
	unsigned int pending_action_freq;
	int pending_action_no_cck;
	int pending_action_without_roc;
	void (*pending_action_tx_status_cb)(struct wpa_supplicant *wpa_s,
					    unsigned int freq, const u8 *dst,
					    const u8 *src, const u8 *bssid,
					    const u8 *data, size_t data_len,
					    enum offchannel_send_action_result
					    result);
	unsigned int roc_waiting_drv_freq;
	int action_tx_wait_time;

#ifdef CONFIG_P2P
	struct p2p_go_neg_results *go_params;
	int create_p2p_iface;
	u8 pending_interface_addr[ETH_ALEN];
	char pending_interface_name[100];
	int pending_interface_type;
	int p2p_group_idx;
	unsigned int pending_listen_freq;
	unsigned int pending_listen_duration;
	enum {
		NOT_P2P_GROUP_INTERFACE,
		P2P_GROUP_INTERFACE_PENDING,
		P2P_GROUP_INTERFACE_GO,
		P2P_GROUP_INTERFACE_CLIENT
	} p2p_group_interface;
	struct p2p_group *p2p_group;
	int p2p_long_listen; /* remaining time in long Listen state in ms */
	char p2p_pin[10];
	int p2p_wps_method;
	u8 p2p_auth_invite[ETH_ALEN];
	int p2p_sd_over_ctrl_iface;
	int p2p_in_provisioning;
	int pending_invite_ssid_id;
	int show_group_started;
	u8 go_dev_addr[ETH_ALEN];
	int pending_pd_before_join;
	u8 pending_join_iface_addr[ETH_ALEN];
	u8 pending_join_dev_addr[ETH_ALEN];
	int pending_join_wps_method;
	int p2p_join_scan_count;
	int force_long_sd;

	/*
	 * Whether cross connection is disallowed by the AP to which this
	 * interface is associated (only valid if there is an association).
	 */
	int cross_connect_disallowed;

	/*
	 * Whether this P2P group is configured to use cross connection (only
	 * valid if this is P2P GO interface). The actual cross connect packet
	 * forwarding may not be configured depending on the uplink status.
	 */
	int cross_connect_enabled;

	/* Whether cross connection forwarding is in use at the moment. */
	int cross_connect_in_use;

	/*
	 * Uplink interface name for cross connection
	 */
	char cross_connect_uplink[100];

	enum {
		P2P_GROUP_REMOVAL_UNKNOWN,
		P2P_GROUP_REMOVAL_REQUESTED,
		P2P_GROUP_REMOVAL_IDLE_TIMEOUT,
		P2P_GROUP_REMOVAL_UNAVAILABLE
	} removal_reason;

	unsigned int p2p_cb_on_scan_complete:1;
#endif /* CONFIG_P2P */

	struct wpa_ssid *bgscan_ssid;
	const struct bgscan_ops *bgscan;
	void *bgscan_priv;

	struct wpa_ssid *connect_without_scan;

	int after_wps;
	unsigned int wps_freq;
	int wps_fragment_size;
	int auto_reconnect_disabled;

	 /* Channel preferences for AP/P2P GO use */
	int best_24_freq;
	int best_5_freq;
	int best_overall_freq;

	struct gas_query *gas;

#ifdef CONFIG_INTERWORKING
	unsigned int fetch_anqp_in_progress:1;
	unsigned int network_select:1;
	unsigned int auto_select:1;
#endif /* CONFIG_INTERWORKING */
	unsigned int drv_capa_known;

	struct {
		struct hostapd_hw_modes *modes;
		u16 num_modes;
		u16 flags;
	} hw;
};


/* wpa_supplicant.c */
int wpa_set_wep_keys(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);

int wpa_supplicant_reload_configuration(struct wpa_supplicant *wpa_s);

const char * wpa_supplicant_state_txt(enum wpa_states state);
int wpa_supplicant_update_mac_addr(struct wpa_supplicant *wpa_s);
int wpa_supplicant_driver_init(struct wpa_supplicant *wpa_s);
int wpa_supplicant_set_suites(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss, struct wpa_ssid *ssid,
			      u8 *wpa_ie, size_t *wpa_ie_len);
void wpa_supplicant_associate(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss,
			      struct wpa_ssid *ssid);
void wpa_supplicant_set_non_wpa_policy(struct wpa_supplicant *wpa_s,
				       struct wpa_ssid *ssid);
void wpa_supplicant_initiate_eapol(struct wpa_supplicant *wpa_s);
void wpa_clear_keys(struct wpa_supplicant *wpa_s, const u8 *addr);
void wpa_supplicant_req_auth_timeout(struct wpa_supplicant *wpa_s,
				     int sec, int usec);
void wpa_supplicant_set_state(struct wpa_supplicant *wpa_s,
			      enum wpa_states state);
struct wpa_ssid * wpa_supplicant_get_ssid(struct wpa_supplicant *wpa_s);
const char * wpa_supplicant_get_eap_mode(struct wpa_supplicant *wpa_s);
void wpa_supplicant_cancel_auth_timeout(struct wpa_supplicant *wpa_s);
void wpa_supplicant_deauthenticate(struct wpa_supplicant *wpa_s,
				   int reason_code);
void wpa_supplicant_disassociate(struct wpa_supplicant *wpa_s,
				 int reason_code);

void wpa_supplicant_enable_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid);
void wpa_supplicant_disable_network(struct wpa_supplicant *wpa_s,
				    struct wpa_ssid *ssid);
void wpa_supplicant_select_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid);
int wpa_supplicant_set_ap_scan(struct wpa_supplicant *wpa_s,
			       int ap_scan);
int wpa_supplicant_set_bss_expiration_age(struct wpa_supplicant *wpa_s,
					  unsigned int expire_age);
int wpa_supplicant_set_bss_expiration_count(struct wpa_supplicant *wpa_s,
					    unsigned int expire_count);
int wpa_supplicant_set_debug_params(struct wpa_global *global,
				    int debug_level, int debug_timestamp,
				    int debug_show_keys);
void free_hw_features(struct wpa_supplicant *wpa_s);

void wpa_show_license(void);

struct wpa_supplicant * wpa_supplicant_add_iface(struct wpa_global *global,
						 struct wpa_interface *iface);
int wpa_supplicant_remove_iface(struct wpa_global *global,
				struct wpa_supplicant *wpa_s);
struct wpa_supplicant * wpa_supplicant_get_iface(struct wpa_global *global,
						 const char *ifname);
struct wpa_global * wpa_supplicant_init(struct wpa_params *params);
int wpa_supplicant_run(struct wpa_global *global);
void wpa_supplicant_deinit(struct wpa_global *global);

int wpa_supplicant_scard_init(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid);
void wpa_supplicant_terminate_proc(struct wpa_global *global);
void wpa_supplicant_rx_eapol(void *ctx, const u8 *src_addr,
			     const u8 *buf, size_t len);
enum wpa_key_mgmt key_mgmt2driver(int key_mgmt);
enum wpa_cipher cipher_suite2driver(int cipher);
void wpa_supplicant_update_config(struct wpa_supplicant *wpa_s);
void wpa_supplicant_clear_status(struct wpa_supplicant *wpa_s);
void wpas_connection_failed(struct wpa_supplicant *wpa_s, const u8 *bssid);
int wpas_driver_bss_selection(struct wpa_supplicant *wpa_s);

/* events.c */
void wpa_supplicant_mark_disassoc(struct wpa_supplicant *wpa_s);
int wpa_supplicant_connect(struct wpa_supplicant *wpa_s,
			   struct wpa_bss *selected,
			   struct wpa_ssid *ssid);
void wpa_supplicant_stop_countermeasures(void *eloop_ctx, void *sock_ctx);
void wpa_supplicant_delayed_mic_error_report(void *eloop_ctx, void *sock_ctx);

/* eap_register.c */
int eap_register_methods(void);

/**
 * Utility method to tell if a given network is a persistent group
 * @ssid: Network object
 * Returns: 1 if network is a persistent group, 0 otherwise
 */
static inline int network_is_persistent_group(struct wpa_ssid *ssid)
{
	return ((ssid->disabled == 2) || ssid->p2p_persistent_group);
}

#endif /* WPA_SUPPLICANT_I_H */
