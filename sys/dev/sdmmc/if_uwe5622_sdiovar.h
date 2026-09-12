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

/*
 * Private, shared definitions for the uwe5622_sdio driver, split
 * across if_uwe5622_sdio.c (match/attach/detach), _transport.c
 * (raw SDIO I/O, PUH framing, card interrupt, activity LED),
 * _fw.c (WCN chip bring-up: SYNC, chip-ID, firmware blob load),
 * _proto.c (SPRDWL command/event protocol), _data.c (TX/RX data
 * path), and _net80211.c (net80211/ifnet glue, sysctls, session
 * bring-up orchestration) - mirrors how sdmmc.c/sdmmc_io.c/
 * sdmmc_mem.c share sdmmcvar.h in this same directory.
 */

#ifndef _DEV_SDMMC_IF_UWE5622_SDIOVAR_H_
#define _DEV_SDMMC_IF_UWE5622_SDIOVAR_H_

#include <sys/types.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/callout.h>
#include <sys/workqueue.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_node.h>

#define UWE_SELFTEST_AT             0
#define UWE_SELFTEST_WIFI_CMD       1
#define UWE_SELFTEST_TIMEOUT_MS     1500
#define UWE_WIFI_CMD_TIMEOUT_MS     2000
#define UWE_WIFI_SCAN_TIMEOUT_MS    9000
#define UWE_WIFI_CONNECT_DRAIN_MS   8000
#define UWE_AUTORUN_SELFTEST        0
#define UWE_ENABLE_NET80211         1
#define UWE_ENABLE_AUTO_WIFI_SESSION 1

/*
 * TX/RX activity LED (uwe5622_sdio_led_touch()/_led_timeout() in
 * if_uwe5622_sdio_transport.c) - shared here since if_uwe5622_sdio.c's
 * uwe5622_sdio_detach() also references UWE_LED_NAME directly.
 */
#define UWE_LED_NAME		"led-1"
#define UWE_LED_DECAY_MS	150

/* Standard CCCR */
#define UWE_SDIO_CCCR_IOEN          0x02
#define UWE_SDIO_CCCR_IORDY         0x03
#define UWE_SDIO_CCCR_INTEN         0x04

/* Unisoc/Spreadtrum SDIO HAL registers */
#define UWE_SDIO_DT_MODE_ADDR       0x0f
#define UWE_SDIO_FBR_SYSADDR0       0x15c
#define UWE_SDIO_FBR_SYSADDR1       0x15d
#define UWE_SDIO_FBR_SYSADDR2       0x15e
#define UWE_SDIO_FBR_SYSADDR3       0x15f
#define UWE_SDIO_PK_MODE_ADDR       0x20

#define UWE_SYNC_ADDR_M3L           0x405E73B0U
#define UWE_CP_START_ADDR           0x40500000U
#define UWE_PACKET_SIZE             (32 * 1024)

#define UWE_CP_RESET_REG            0x40088288U
#define UWE_RESET_BIT               (1U << 0)

#define UWE_SYNC_TSX_DAC_DATA_OFF   0x18U
#define UWE_SYNC_SDIO_CONFIG_OFF    0x1cU

#define UWE_CHIPID_REG_M3_M3L       0x4083c208U
#define UWE_CHIPID_REG_M3E          0x4082c208U

#define UWE_WCN_CHIPID_MASK         0xFFFFF000U
#define UWE_MARLIN3_AA_CHIPID       0x23550000U
#define UWE_MARLIN3L_AA_CHIPID      0x2355B000U
#define UWE_MARLIN3E_AA_CHIPID      0x56630000U

#define UWE_SYNC_IN_PROGRESS        0xF0F0F0F0U
#define UWE_SYNC_CALI_WAITING       0xF0F0F0F1U
#define UWE_SYNC_CALI_WRITE_DONE    0xF0F0F0F2U
#define UWE_SYNC_CALI_FINISHED      0xF0F0F0F3U
#define UWE_SYNC_SDIO_REINIT_DONE   0xF0F0F0F4U
#define UWE_SYNC_SDIO_IS_READY      0xF0F0F0F5U
#define UWE_SYNC_VERIFY_WAITING     0xF0F0F0F6U
#define UWE_SYNC_VERIFY_WRITE_DONE  0xF0F0F0F7U
#define UWE_SYNC_VERIFY_FINISHED    0xF0F0F0F8U
#define UWE_SYNC_ALL_FINISHED       0xF0F0F0FFU

#define UWE_PKT_TYPE_CTRL           0x0
#define UWE_PKT_TYPE_CMD            0x0
#define UWE_PKT_TYPE_EVENT          0x1
#define UWE_PKT_TYPE_DATA           0x2
#define UWE_PKT_SUBTYPE_AT_CMD      0x0
#define UWE_PKT_SUBTYPE_AT_REPLY    0x0
#define UWE_PKT_SUBTYPE_AT_STATUS   0x1
#define UWE_PKT_SUBTYPE_LOG         0x3
#define UWE_PKT_STAT_SLOTS          32

#define UWE_PKT_EOF_RAW             0x00800000U

#define UWE_PKT_RX_BUFSZ            2048
#define UWE_AT_REPLY_BUFSZ          1024
#define UWE_LOG_PREVIEW_BUFSZ       96
#define UWE_LOG_ASCII_EXTRACT_BUFSZ 256
#define UWE_AT_POLL_DELAY_US        20000
#define UWE_RX_IDLE_POLL_DELAY_US    2000
#define UWE_RX_QUIET_POLL_DELAY_US  10000
#define UWE_RX_IDLE_BACKOFF_POLLS       4
#define UWE_AT_DEFAULT_TIMEOUT_MS   1000

#define UWE_WIFI_CTX_INVALID        0xff
#define UWE_WIFI_MAC_LEN            6
#define UWE_WIFI_MAX_REPLY          512
#define UWE_WIFI_SCAN_MAX_PAYLOAD   256
#define UWE_WIFI_CMD_TX_PUH_SUBTYPE 8
#define UWE_WIFI_CMD_RX_PUH_SUBTYPE 10
/*
 * unisocwcn/sdio/sdiohal_common.c: RX channel = subtype + 12 (SDIO_CHN_TX_NUM).
 * wl_intf.c registers SDIO_RX_CMD_PORT=22 (-> subtype 10, matches
 * UWE_WIFI_CMD_RX_PUH_SUBTYPE above - independently confirms this scheme),
 * SDIO_RX_PKT_LOG_PORT=23 (-> subtype 11), SDIO_RX_DATA_PORT=24
 * (-> subtype 12). Real inbound DATA frames arrive on a DIFFERENT subtype
 * than CMD replies/events, not distinguished via inner common_hdr.type
 * under the same subtype=10 branch as previously assumed.
 */
#define UWE_WIFI_DATA_RX_PUH_SUBTYPE 12
/*
 * wl_intf.h: #define SDIO_TX_DATA_PORT 10 (vs #define SDIO_TX_CMD_PORT 8,
 * which matches this driver's own already-working
 * UWE_WIFI_CMD_TX_PUH_SUBTYPE=8 exactly) - outgoing DATA frames need a
 * DIFFERENT wire subtype than outgoing CMD frames, not the same one as
 * this driver previously assumed.
 */
#define UWE_WIFI_DATA_TX_PUH_SUBTYPE 10
/* wl_intf.c's sprdwl_intf_fill_msdu_dscr(): #define DSCR_LEN 11 */
#define UWE_TX_MSDU_DSCR_LEN 11
/*
 * sprdwl.h: #define FOUR_BYTES_ALIGN_OFFSET 3, consumed at 3 independent
 * call sites (wl_intf.c sprdwl_tx_data_list(), tx_msg.c
 * sprdwl_queue_data_msg_buf() and a third) as "tran_data +
 * FOUR_BYTES_ALIGN_OFFSET" whenever OTT_UWE is defined. unisocwifi/Makefile
 * defines OTT_UWE unconditionally (ccflags-y += -DOTT_UWE) for this exact
 * chip's obj-$(CONFIG_WLAN_UWE5622) module - not gated by any variant
 * #ifdef, unlike UWE5621_FTR which genuinely is chip-specific and does NOT
 * apply here. This 3-byte gap sits between the outer transport header and
 * the tx_msdu_dscr.
 */
#define UWE_TX_OTT_PREFIX_LEN 3

#define UWE_DUMP_MAX_BYTES          48

/* reverse engineered from Linux unisocwifi/msg.h */
#define UWE_SPRDWL_TYPE_CMD         0
#define UWE_SPRDWL_TYPE_EVENT       1
#define UWE_SPRDWL_TYPE_DATA        2
#define UWE_SPRDWL_HEAD_NORSP       0
#define UWE_SPRDWL_HEAD_RSP         1

/* reverse engineered from Linux unisocwifi/cmdevt.h */
enum uwe_wifi_cmd_id {
	UWE_WIFI_CMD_ERR = 0,
	UWE_WIFI_CMD_GET_INFO = 1,
	UWE_WIFI_CMD_SET_REGDOM = 2,
	UWE_WIFI_CMD_OPEN = 3,
	UWE_WIFI_CMD_CLOSE = 4,
	UWE_WIFI_CMD_POWER_SAVE = 5,
	UWE_WIFI_CMD_SET_PARAM = 6,
	UWE_WIFI_CMD_SET_CHANNEL = 7,
	UWE_WIFI_CMD_REQ_LTE_CONCUR = 8,
	UWE_WIFI_CMD_SYNC_VERSION = 9,
	UWE_WIFI_CMD_CONNECT = 10,
	UWE_WIFI_CMD_SCAN = 11,
	UWE_WIFI_CMD_SCHED_SCAN = 12,
	UWE_WIFI_CMD_KEY = 14,
	UWE_WIFI_CMD_SET_IE = 25,
	UWE_WIFI_CMD_NOTIFY_IP_ACQUIRED = 26,
	UWE_WIFI_CMD_BA = 68,
	UWE_WIFI_CMD_DOWNLOAD_INI = 76
};

enum uwe_wifi_event_id {
	UWE_WIFI_EVENT_CONNECT = 0x80,
	UWE_WIFI_EVENT_DISCONNECT = 0x81,
	UWE_WIFI_EVENT_SCAN_DONE = 0x82,
	UWE_WIFI_EVENT_MGMT_FRAME = 0x83,
	UWE_WIFI_EVENT_GSCAN_FRAME = 0x88,
	UWE_WIFI_EVENT_SDIO_FLOWCON = 0xb3,
	UWE_WIFI_EVENT_BA = 0xf3,
	UWE_WIFI_EVENT_STA_LUT_INDEX = 0xf5,
	UWE_WIFI_EVENT_FW_PWR_DOWN = 0xfa
};

#define UWE_SPRDWL_SCAN_DONE        1
#define UWE_SPRDWL_SCHED_SCAN_DONE  2
#define UWE_SPRDWL_SCAN_ERROR       3
#define UWE_SPRDWL_GSCAN_DONE       4

#define UWE_SPRDWL_CAPA_5G          __BIT(0)
#define UWE_SPRDWL_CAPA_MCC         __BIT(1)
#define UWE_SPRDWL_CAPA_ACL         __BIT(2)
#define UWE_SPRDWL_CAPA_AP_SME      __BIT(3)
#define UWE_SPRDWL_CAPA_PMK_OKC     __BIT(4)
#define UWE_SPRDWL_CAPA_11R_ROAM    __BIT(5)
#define UWE_SPRDWL_CAPA_SCHED_SCAN  __BIT(6)
#define UWE_SPRDWL_CAPA_TDLS        __BIT(7)
#define UWE_SPRDWL_CAPA_MC_FILTER   __BIT(8)
#define UWE_SPRDWL_CAPA_NS_OFFLOAD  __BIT(9)
#define UWE_SPRDWL_CAPA_RA_OFFLOAD  __BIT(10)
#define UWE_SPRDWL_CAPA_LL_STATS    __BIT(11)
#define UWE_SPRDWL_CAPA_NAN         __BIT(12)
#define UWE_SPRDWL_CAPA_CONFIG_NDO  __BIT(13)
#define UWE_SPRDWL_CAPA_GSCAN       __BIT(17)
#define UWE_SPRDWL_CAPA_BATCH_SCAN  __BIT(18)
#define UWE_SPRDWL_CAPA_SCAN_RAND   __BIT(22)
#define UWE_SPRDWL_CAPA_TX_POWER    __BIT(28)

#define UWE_SEC1_LEN                24

#ifdef UWE_DEBUG
#define UWE_DPRINTF(sc, fmt, ...) \
	aprint_normal_dev((sc)->sc_dev, fmt, ##__VA_ARGS__)
#else
#define UWE_DPRINTF(sc, fmt, ...) do { } while (0)
#endif

enum uwe_chip_model {
	UWE_CHIP_INVALID = 0,
	UWE_CHIP_MARLIN3,
	UWE_CHIP_MARLIN3L,
	UWE_CHIP_MARLIN3E
};

struct uwe_pkt_header {
	uint8_t type;
	uint8_t subtype;
	uint8_t eof;
	uint8_t csum;
	uint8_t pad;
	uint16_t len;
};

struct uwe_pkt_stat {
	bool used;
	uint8_t type;
	uint8_t subtype;
	uint32_t count;
	uint32_t last_len;
};

struct uwe_sprdwl_common_hdr {
	uint8_t raw;
} __packed;

struct uwe_sprdwl_cmd_hdr {
	struct uwe_sprdwl_common_hdr common;
	uint8_t cmd_id;
	uint16_t plen_le;
	uint32_t mstime_le;
	int8_t status;
	uint8_t rsp_cnt;
	uint8_t reserv[2];
} __packed;

struct uwe_sprdwl_cmd_open {
	uint8_t mode;
	uint8_t reserved;
	uint8_t mac[UWE_WIFI_MAC_LEN];
} __packed;

struct uwe_sprdwl_cmd_close {
	uint8_t mode;
} __packed;

struct uwe_sprdwl_cmd_scan {
	uint32_t channels_le;
	uint32_t reserved_le;
	uint16_t ssid_len_le;
	uint8_t ssid[0];
} __packed;

struct uwe_sprdwl_5g_tail {
	uint16_t n_5g_chn_le;
	uint16_t chns_le[0];
} __packed;

static const uint8_t uwe_5ghz_channels[] = {
	36, 40, 44, 48,
	52, 56, 60, 64,
	100, 104, 108, 112, 116, 120, 124, 128,
	132, 136, 140, 144,
	149, 153, 157, 161, 165
};

struct uwe_sprdwl_event_scan_done {
	uint8_t type;
} __packed;

struct uwe_sprdwl_cmd_scan_min {
	uint32_t channels_le;
	uint32_t reserved_le;
	uint16_t ssid_len_le;
	uint16_t n_5g_chn_le;
} __packed;

/*
 * WIFI_CMD_CONNECT payload, matches vendor sprdwl_cmd_connect (verified
 * against orangepi-xunlong/linux-orangepi cmdevt.h). Firmware performs the
 * entire auth/assoc/4-way-handshake internally; the host only supplies
 * target SSID/BSSID/channel and the pre-derived 256-bit PMK.
 */
#define UWE_SPRDWL_CIPHER_NONE		0
#define UWE_SPRDWL_CIPHER_WEP40		1
#define UWE_SPRDWL_CIPHER_WEP104	2
#define UWE_SPRDWL_CIPHER_TKIP		3
#define UWE_SPRDWL_CIPHER_CCMP		4	/* confirmed from vendor src */

/*
 * Confirmed against vendor orangepi-xunlong/linux-orangepi cfg80211.h.
 * Earlier guess had AKM_SUITE_PSK=1, which is actually SPRDWL_AKM_SUITE_8021X
 * (enterprise auth) - firmware happily ACKed CONNECT with the wrong AKM
 * (it just echoes cmd success), but the AP silently rejected the resulting
 * association since we were asking for 802.1X, not PSK, while supplying a
 * PSK. Explains "CONNECT event success" on our side + device never showing
 * up as associated on the AP.
 */
#define UWE_SPRDWL_AUTH_OPEN		0
#define UWE_SPRDWL_AUTH_SHARED		1
#define UWE_SPRDWL_AKM_SUITE_NONE	0
#define UWE_SPRDWL_AKM_SUITE_8021X	1
#define UWE_SPRDWL_AKM_SUITE_PSK	2

/*
 * cfg80211.c ORs this into pairwise_cipher/group_cipher/key_mgmt to mark
 * them as explicitly configured (SPRDWL_VALID_CONFIG=0x80 in cmdevt.h) -
 * without it firmware likely treats those fields as unset/default rather
 * than honoring the CCMP/PSK values we put in the low bits.
 */
#define UWE_SPRDWL_VALID_CONFIG	0x80

#define UWE_WPA_VERSION_1		1	/* NL80211_WPA_VERSION_1, passed through raw */
#define UWE_WPA_VERSION_2		2	/* NL80211_WPA_VERSION_2, passed through raw */

#define UWE_WLAN_MAX_KEY_LEN		32
#define UWE_IEEE80211_MAX_SSID_LEN	32
#define UWE_WIFI_PSK_CFG_LEN		64

/*
 * SPRDWL_TYPE_DATA wire header, verified against vendor
 * orangepi-xunlong/linux-orangepi msg.h struct sprdwl_data_hdr. Firmware
 * handles all 802.11 framing internally - the payload is a plain,
 * already-assembled 802.3 ethernet frame, no conversion needed in either
 * direction - BUT on TX it does not follow this header immediately: see
 * UWE_SPRDWL_DATA_OFFSET below.
 */
struct uwe_sprdwl_data_hdr {
	struct uwe_sprdwl_common_hdr common;
	uint8_t info1;
	uint16_t plen_le;
	uint8_t flow[4];
} __packed;

/*
 * wl_intf.c's sprdwl_start_xmit() calls sprdwl_send_data(vif, msg, skb, 2)
 * for this chip family (non-UWE5621_FTR, i.e. the struct sprdwl_data_hdr
 * branch above) - msg.h's own #define SPRDWL_DATA_OFFSET is 2. txrx.c's
 * sprdwl_send_data() does skb_push(skb, sizeof(*hdr) + offset), i.e. a
 * mandatory 2-byte gap between the 8-byte header and the actual Ethernet
 * payload on TX, included in plen. Confirmed real vendor source, not
 * inferred.
 */
#define UWE_SPRDWL_DATA_OFFSET		2

struct uwe_sprdwl_cmd_connect {
	uint32_t wpa_versions_le;
	uint8_t bssid[UWE_WIFI_MAC_LEN];
	uint8_t channel;
	uint8_t auth_type;
	uint8_t pairwise_cipher;
	uint8_t group_cipher;
	uint8_t key_mgmt;
	uint8_t mfp_enable;
	uint8_t psk_len;
	uint8_t ssid_len;
	uint8_t psk[UWE_WLAN_MAX_KEY_LEN];
	uint8_t ssid[UWE_IEEE80211_MAX_SSID_LEN];
} __packed;

struct uwe_sprdwl_cmd_set_ie {
	uint8_t type;
	uint16_t len_le;
	uint8_t data[];
} __packed;

#define UWE_SPRDWL_IE_ASSOC_REQ	3

/* enum SPRDWL_SUBCMD: GET=1, SET=2, ADD=3, DEL=4. */
#define UWE_SPRDWL_KEY_ADD	3
#define UWE_SPRDWL_KEY_DEL	4

struct uwe_sprdwl_cmd_add_key {
	uint8_t key_index;
	uint8_t pairwise;
	uint8_t mac[UWE_WIFI_MAC_LEN];
	uint8_t keyseq[16];
	uint8_t cipher_type;
	uint8_t key_len;
	uint8_t value[];
} __packed;

struct uwe_sprdwl_cmd_del_key {
	uint8_t key_index;
	uint8_t pairwise;
	uint8_t mac[UWE_WIFI_MAC_LEN];
} __packed;

struct uwe_wiphy_sec2 {
	uint16_t ht_cap_info;
	uint16_t ampdu_para;
	uint8_t ht_mcs_set[16];
	uint32_t vht_cap_info;
	uint8_t vht_mcs_set[8];
	uint32_t antenna_tx;
	uint32_t antenna_rx;
	uint8_t retry_short;
	uint8_t retry_long;
	uint16_t reserved;
	uint32_t frag_threshold;
	uint32_t rts_threshold;
} __packed;

struct uwe_sprdwl_cmd_fw_info {
	uint32_t chip_model_le;
	uint32_t chip_version_le;
	uint32_t fw_version_le;
	uint32_t fw_std_le;
	uint32_t fw_capa_le;
	uint8_t max_ap_assoc;
	uint8_t max_acl_mac_addrs;
	uint8_t max_mc_mac_addrs;
	uint8_t wnm_ft_support;
	struct uwe_wiphy_sec2 wiphy_sec2;
	uint8_t mac_addr[UWE_WIFI_MAC_LEN];
	uint8_t credit_capa;
} __packed;

struct uwe_ieee80211_freq_range {
	uint32_t start_freq_khz;
	uint32_t end_freq_khz;
	uint32_t max_bandwidth_khz;
};

struct uwe_ieee80211_power_rule {
	uint32_t max_antenna_gain;
	uint32_t max_eirp;
};

struct uwe_ieee80211_reg_rule {
	struct uwe_ieee80211_freq_range freq_range;
	struct uwe_ieee80211_power_rule power_rule;
	uint32_t flags;
	uint32_t dfs_cac_ms;
};

struct uwe_sprdwl_ieee80211_regdomain {
	uint32_t n_reg_rules;
	char alpha2[2];
	struct uwe_ieee80211_reg_rule reg_rules[0];
};

struct uwe5622_sdio_softc {
	device_t sc_dev;
	struct sdmmc_function *sc_sf;
	bool sc_enabled;
	bool sc_sync_cfg_written;
	bool sc_transport_ready;
	bool sc_intr_enabled;
	/*
	 * sunxi_mmc.c fully implements sdmmc(4)'s generic SDIO card-interrupt
	 * plumbing (card_enable_intr/card_intr_ack wired to the real
	 * SUNXI_MMC_INT_SDIO_INT hardware IRQ bit) - sdmmc_intr_establish()
	 * arms the host-side listener; the card-side per-function enable
	 * this driver already does via uwe5622_sdio_enable_func_intr() is
	 * the other half of the same handshake. Without calling
	 * sdmmc_intr_establish(), the card's INT assertions were previously
	 * just ignored and the RX kthread relied purely on polling.
	 */
	void *sc_sdio_ih;
	kmutex_t sc_rx_cv_lock;
	kcondvar_t sc_rx_cv;

	/*
	 * TX/RX activity indicator on the board's "led-1" (green, STATUS)
	 * LED: touched (turned on, callout pushed back) from the TX send
	 * path and RX dispatch path, decays to off if not re-touched
	 * within UWE_LED_DECAY_MS. sc_led_on is a benign-race hint used
	 * only to avoid redundant led_set_by_name() calls (which take the
	 * LED subsystem's own lock) on every packet; the real on/off state
	 * lives in the LED subsystem itself.
	 */
	callout_t sc_led_co;
	bool sc_led_on;

	uint32_t sc_chipid;
	enum uwe_chip_model sc_chip_model;

	bool sc_at_reply_ready;
	uint8_t sc_at_reply_subtype;
	size_t sc_at_reply_len;
	char sc_at_reply[UWE_AT_REPLY_BUFSZ];

	bool sc_log_seen;
	size_t sc_log_preview_len;
	uint8_t sc_log_preview[UWE_LOG_PREVIEW_BUFSZ];

	struct uwe_pkt_stat sc_pkt_stats[UWE_PKT_STAT_SLOTS];
	uint32_t sc_rx_total_packets;
	uint32_t sc_rx_unhandled_packets;

	bool sc_wifi_reply_ready;
	bool sc_wifi_scan_done;
	bool sc_wifi_scan_aborted;
	bool sc_wifi_connect_done;
	/*
	 * cmd_id of the one outstanding synchronous wifi-cmd wait, or 0
	 * (UWE_WIFI_CMD_ERR, never a real request) if none. Set by
	 * uwe5622_sdio_send_wifi_cmd() only for need_rsp=true sends, right
	 * before the bus write - closes the window between send and the
	 * corresponding uwe5622_sdio_wait_wifi_reply() call. Checked by
	 * uwe5622_sdio_handle_wifi_cmd_payload(), which only delivers a
	 * reply into the sc_wifi_reply_* mailbox below when its cmd_id
	 * matches: both the dedicated RX kthread and a waiter's own inline
	 * uwe5622_sdio_rx_consume_once() race to dispatch incoming
	 * packets, and unconditionally overwriting the mailbox for every
	 * reply (the pre-2026-08-26 behavior) let an unrelated command's
	 * reply (e.g. firmware's own auto-ack for a fire-and-forget BA
	 * send) clobber an already-arrived-but-not-yet-read matching
	 * reply. Assumes at most one need_rsp=true wifi-cmd is ever
	 * in-flight at a time (already true of this driver's protocol
	 * sequencing); fire-and-forget (need_rsp=false) sends must never
	 * touch this field.
	 */
	uint8_t sc_wifi_pending_cmd;
	uint8_t sc_wifi_reply_cmd_id;
	uint8_t sc_wifi_reply_ctx_id;
	int8_t sc_wifi_reply_status;
	size_t sc_wifi_reply_len;
	uint8_t sc_wifi_reply[UWE_WIFI_MAX_REPLY];
	uint8_t sc_wifi_ctx_id;
	uint32_t sc_fw_version;
	uint32_t sc_fw_std;
	uint32_t sc_fw_capa;
	uint8_t sc_credit_capa;
	uint8_t sc_ott_supt;
	bool sc_fw_tx_asserted;
	/*
	 * vendor tx_msg.c sprdwl_sdio_process_credit()/sprdwl_fc_get_send_num():
	 * 4 independent per-"color" credit pools, replenished by FLOWCON
	 * event payload bytes (atomic_add), consumed one-per-send by the
	 * host BEFORE writing to SDIO - not just logged. This driver only
	 * ever uses color/queue 0 (single STA-mode vif, matching vendor's
	 * sprdwl_fc_find_color_per_mode() assigning the first free slot),
	 * so only sc_tx_credit[0] is actually drawn down; the other 3
	 * slots are tracked for completeness/future use but never gate
	 * anything here.
	 */
	volatile uint32_t sc_tx_credit[4];
	uint8_t sc_fw_mac[UWE_WIFI_MAC_LEN];
	bool sc_wifi_opened;
	bool sc_wifi_session_ready;
	bool sc_scan_publish_active;

	struct ethercom sc_ec;
	struct ieee80211com sc_ic;
	bool sc_net80211_attached;

	struct sysctllog *sc_sysctllog;
	char sc_wifi_psk_cfg[UWE_WIFI_PSK_CFG_LEN + 1];

	struct workqueue *sc_tx_wq;
	struct work sc_tx_work;
	volatile u_int sc_tx_pending;
	volatile uint32_t sc_tx_start_calls;
	volatile uint32_t sc_tx_schedule_claims;
	volatile uint32_t sc_tx_schedule_busy;
	volatile uint32_t sc_tx_worker_runs;
	volatile uint32_t sc_tx_dequeued;
	volatile uint32_t sc_tx_ok;
	volatile uint32_t sc_tx_error;
	volatile uint32_t sc_tx_no_credit;
	volatile uint32_t sc_tx_no_lut;
	volatile uint32_t sc_tx_rechecks;
	volatile uint32_t sc_tx_lost_wakeup_suspect;
	volatile uint32_t sc_tx_credit_waits;
	volatile uint32_t sc_tx_color_used[4];
	volatile bool sc_tx_waiting_credit;
	volatile uint32_t sc_ba_events;
	volatile uint32_t sc_ba_rsp_ok;
	volatile uint32_t sc_ba_rsp_error;
	volatile uint32_t sc_ip_notify_attempts;
	volatile uint32_t sc_ip_notify_ok;
	volatile uint32_t sc_ip_notify_error;
	bool sc_ipv4_notified;
	uint8_t sc_notified_ipv4[4];

	kmutex_t sc_bus_lock;
	lwp_t *sc_rx_lwp;
	volatile bool sc_rx_stop;

	uint8_t sc_wifi_connect_chan;
	uint8_t sc_wifi_connect_bssid[UWE_WIFI_MAC_LEN];
	bool sc_verbose;

	bool sc_sta_lut_valid;
	uint8_t sc_sta_lut_index;
	uint8_t sc_sta_ra[UWE_WIFI_MAC_LEN];
};
#define sc_if sc_ec.ec_if

struct uwe_fw_head {
	char magic[4];
	uint32_t version;
	uint32_t img_count;
};

struct uwe_fw_imageinfo {
	char tag[4];
	uint32_t offset;
	uint32_t size;
};

int  uwe5622_sdio_match(device_t, cfdata_t, void *);
void uwe5622_sdio_attach(device_t, device_t, void *);
int  uwe5622_sdio_detach(device_t, int);

void     uwe5622_sdio_write_sysaddr(struct uwe5622_sdio_softc *, uint32_t);
uint32_t uwe5622_sdio_read_dt_port_read4(struct uwe5622_sdio_softc *);
uint32_t uwe5622_sdio_read_dt_port_multi4(struct uwe5622_sdio_softc *);
uint32_t uwe5622_sdio_readl_read4(struct uwe5622_sdio_softc *, uint32_t);
uint32_t uwe5622_sdio_readl_multi(struct uwe5622_sdio_softc *, uint32_t);

int uwe5622_sdio_dt_read(struct uwe5622_sdio_softc *, uint32_t, void *, size_t);
int uwe5622_sdio_dt_write(struct uwe5622_sdio_softc *, uint32_t,
    const void *, size_t);
int uwe5622_sdio_fw_write_chunked(struct uwe5622_sdio_softc *,
    uint32_t, const void *, size_t);

int uwe5622_sdio_reg_read32(struct uwe5622_sdio_softc *, uint32_t, uint32_t *);
int uwe5622_sdio_reg_write32(struct uwe5622_sdio_softc *, uint32_t, uint32_t);
int uwe5622_sdio_start_run(struct uwe5622_sdio_softc *);

uint32_t uwe5622_sdio_build_sdio_config(void);
int uwe5622_sdio_write_cali_done(struct uwe5622_sdio_softc *);
int uwe5622_sdio_enable_func_intr(struct uwe5622_sdio_softc *);
int uwe5622_sdio_intr(void *);
bool uwe5622_sdio_get_own_ipv4(struct uwe5622_sdio_softc *, uint8_t[4]);
void uwe5622_sdio_led_touch(struct uwe5622_sdio_softc *);
void uwe5622_sdio_led_timeout(void *);
int uwe5622_sdio_transport_init(struct uwe5622_sdio_softc *);
int uwe5622_sdio_wait_for_sync(struct uwe5622_sdio_softc *, uint32_t, int);

int uwe5622_sdio_pkt_read(struct uwe5622_sdio_softc *, void *, size_t);
int uwe5622_sdio_pkt_write(struct uwe5622_sdio_softc *, const void *, size_t);
void uwe5622_sdio_pkt_stat_note(struct uwe5622_sdio_softc *,
    uint8_t, uint8_t, uint16_t);
void uwe5622_sdio_pkt_stat_dump(struct uwe5622_sdio_softc *);

int uwe5622_sdio_send_cmd(struct uwe5622_sdio_softc *, uint8_t,
    const void *, size_t) __unused;
int uwe5622_sdio_rx_consume_buf(struct uwe5622_sdio_softc *, uint8_t *,
    bool *);
int uwe5622_sdio_rx_consume_once(struct uwe5622_sdio_softc *, bool *);
void uwe5622_sdio_rx_parse_frame(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t, uint32_t);

void uwe5622_sdio_run_cmd(struct uwe5622_sdio_softc *,
    const char *) __unused;
void uwe5622_sdio_run_wifi_cmd_matrix(struct uwe5622_sdio_softc *) __unused;

int uwe5622_at_cmd(struct uwe5622_sdio_softc *, const char *,
    char *, size_t, int) __unused;
int uwe5622_at_cmd_ex(struct uwe5622_sdio_softc *,
    const char *, char *, size_t, uint8_t *, int);

void uwe5622_sdio_try_cmd(struct uwe5622_sdio_softc *,
    const char *) __unused;

int uwe5622_sdio_ensure_wifi_session(struct uwe5622_sdio_softc *);
int uwe5622_sdio_bridge_scan_request(struct uwe5622_sdio_softc *);

const struct uwe_fw_imageinfo *uwe5622_find_wcnm_image(
    const uint8_t *, size_t, const char *);
int uwe5622_fw_try_parse_wcnm_3lab(const uint8_t *, size_t,
    const uint8_t **, size_t *);
void uwe5622_fw_probe_blob(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);

void uwe5622_sdio_dt_dump(struct uwe5622_sdio_softc *, uint32_t, size_t,
    const char *);
void uwe5622_sdio_dump_payload_ascii(const uint8_t *, size_t) __unused;
void uwe5622_sdio_dump_bytes(struct uwe5622_sdio_softc *,
    const char *, const void *, size_t);
void uwe5622_sdio_decode_puh(struct uwe5622_sdio_softc *,
    uint32_t, const char *) __unused;
void uwe5622_sdio_log_printable_segments(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);
uint32_t uwe5622_sdio_pkt_build_raw(uint8_t, uint8_t, uint8_t,
    size_t, uint8_t);
void uwe5622_sdio_pkt_decode(uint32_t, struct uwe_pkt_header *);
void uwe5622_sdio_handle_at_payload(struct uwe5622_sdio_softc *,
    uint8_t, const uint8_t *, size_t);
void uwe5622_sdio_handle_log_payload(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);

enum uwe_chip_model uwe5622_sdio_classify_chipid(uint32_t);
const char *uwe5622_sdio_chip_name(enum uwe_chip_model);

void uwe5622_sdio_selftest(struct uwe5622_sdio_softc *) __unused;

uint8_t uwe5622_sdio_common_build(uint8_t, uint8_t, uint8_t, uint8_t);
uint8_t uwe5622_sdio_common_type(uint8_t);
uint8_t uwe5622_sdio_common_rsp(uint8_t);
uint8_t uwe5622_sdio_common_ctx(uint8_t);
int uwe5622_sdio_send_wifi_cmd(struct uwe5622_sdio_softc *, uint8_t,
    uint8_t, const void *, size_t, bool);
int uwe5622_sdio_tx_data(struct uwe5622_sdio_softc *, struct mbuf *);
void uwe5622_sdio_handle_data_payload(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);
void uwe5622_sdio_handle_wifi_cmd_payload(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);
void uwe5622_sdio_handle_wifi_event_payload(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);
int uwe5622_sdio_wait_wifi_reply(struct uwe5622_sdio_softc *, uint8_t,
    int, uint8_t *, size_t *, uint8_t *, int8_t *);
int uwe5622_sdio_wifi_cmd_open(struct uwe5622_sdio_softc *, uint8_t,
    const uint8_t *, uint8_t *);
int uwe5622_sdio_wifi_cmd_close(struct uwe5622_sdio_softc *, uint8_t,
    uint8_t);
int uwe5622_sdio_wifi_cmd_get_info(struct uwe5622_sdio_softc *, uint8_t);
int uwe5622_sdio_wifi_cmd_scan(struct uwe5622_sdio_softc *, uint8_t,
    uint32_t);
int uwe5622_sdio_wifi_cmd_connect(struct uwe5622_sdio_softc *,
    uint8_t, const uint8_t *, uint8_t, const char *, const char *);

struct uwe5622_ssid_lookup {
	struct uwe5622_sdio_softc *sc;
	const char *ssid;
	size_t ssid_len;
	bool found;
	uint8_t bssid[UWE_WIFI_MAC_LEN];
	uint8_t chan;
	uint8_t best_rssi;
};
void uwe5622_sdio_ssid_lookup_cb(void *, struct ieee80211_node *);
void uwe5622_sdio_sysctl_attach(struct uwe5622_sdio_softc *);
int uwe5622_sdio_sysctl_psk(SYSCTLFN_PROTO);
int uwe5622_sdio_sysctl_dump_stats(SYSCTLFN_PROTO);
// static int uwe5622_sdio_wifi_cmd_exchange(struct uwe5622_sdio_softc *,
//     uint8_t, uint8_t, const void *, size_t, void *, size_t *,
//     int, uint8_t *, int *) __unused;	
const char *uwe5622_sdio_wifi_cmd_name(uint8_t);
const char *uwe5622_sdio_wifi_event_name(uint8_t);
void uwe5622_sdio_dump_fw_info(struct uwe5622_sdio_softc *);
const char *uwe5622_sdio_ioctl_name(u_long);

void uwe5622_sdio_net80211_attach(struct uwe5622_sdio_softc *);
void uwe5622_sdio_net80211_detach(struct uwe5622_sdio_softc *);
void uwe5622_sdio_net80211_setup_channels(struct uwe5622_sdio_softc *);
void uwe5622_sdio_net80211_sync_mac(struct uwe5622_sdio_softc *);
void uwe5622_sdio_try_log_mgmt_frame(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t);
bool uwe5622_sdio_parse_mgmt_ies(const uint8_t *, size_t, char *,
    size_t, int *);
void uwe5622_sdio_publish_scan_result(struct uwe5622_sdio_softc *,
    const uint8_t *, size_t, int, int);
void uwe5622_sdio_begin_net80211_scan(struct uwe5622_sdio_softc *) __unused;
void uwe5622_sdio_finish_net80211_scan(struct uwe5622_sdio_softc *);
int uwe5622_sdio_media_change(struct ifnet *);
void uwe5622_sdio_media_status(struct ifnet *, struct ifmediareq *);
int uwe5622_sdio_if_init(struct ifnet *);
void uwe5622_sdio_if_stop(struct ifnet *, int);
int uwe5622_sdio_if_ioctl(struct ifnet *, u_long, void *);
void uwe5622_sdio_if_start(struct ifnet *);
int uwe5622_sdio_key_set(struct ieee80211com *,
    const struct ieee80211_key *, const uint8_t[IEEE80211_ADDR_LEN]);
int uwe5622_sdio_key_delete(struct ieee80211com *,
    const struct ieee80211_key *);
void uwe5622_sdio_tx_work(struct work *, void *);
void uwe5622_sdio_rx_thread(void *);

/*
 * These three were never forward-declared in the original monolithic file
 * (each was defined early enough there that no prior declaration was ever
 * needed) but now need one here since they're called from a different file
 * than the one they're defined in.
 */
int uwe5622_sdio_wifi_set_regdom_world(struct uwe5622_sdio_softc *, uint8_t) __unused;
int uwe5622_sdio_wifi_download_ini(struct uwe5622_sdio_softc *, uint8_t);
void uwe5622_sdio_if_watchdog(struct ifnet *);

#endif /* !_DEV_SDMMC_IF_UWE5622_SDIOVAR_H_ */
