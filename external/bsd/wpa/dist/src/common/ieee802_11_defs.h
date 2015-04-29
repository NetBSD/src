/*
 * IEEE 802.11 Frame type definitions
 * Copyright (c) 2002-2015, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2007-2008 Intel Corporation
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef IEEE802_11_DEFS_H
#define IEEE802_11_DEFS_H

/* IEEE 802.11 defines */

#define WLAN_FC_PVER		0x0003
#define WLAN_FC_TODS		0x0100
#define WLAN_FC_FROMDS		0x0200
#define WLAN_FC_MOREFRAG	0x0400
#define WLAN_FC_RETRY		0x0800
#define WLAN_FC_PWRMGT		0x1000
#define WLAN_FC_MOREDATA	0x2000
#define WLAN_FC_ISWEP		0x4000
#define WLAN_FC_ORDER		0x8000

#define WLAN_FC_GET_TYPE(fc)	(((fc) & 0x000c) >> 2)
#define WLAN_FC_GET_STYPE(fc)	(((fc) & 0x00f0) >> 4)

#define WLAN_INVALID_MGMT_SEQ   0xFFFF

#define WLAN_GET_SEQ_FRAG(seq) ((seq) & (BIT(3) | BIT(2) | BIT(1) | BIT(0)))
#define WLAN_GET_SEQ_SEQ(seq) \
	(((seq) & (~(BIT(3) | BIT(2) | BIT(1) | BIT(0)))) >> 4)

#define WLAN_FC_TYPE_MGMT		0
#define WLAN_FC_TYPE_CTRL		1
#define WLAN_FC_TYPE_DATA		2

/* management */
#define WLAN_FC_STYPE_ASSOC_REQ		0
#define WLAN_FC_STYPE_ASSOC_RESP	1
#define WLAN_FC_STYPE_REASSOC_REQ	2
#define WLAN_FC_STYPE_REASSOC_RESP	3
#define WLAN_FC_STYPE_PROBE_REQ		4
#define WLAN_FC_STYPE_PROBE_RESP	5
#define WLAN_FC_STYPE_BEACON		8
#define WLAN_FC_STYPE_ATIM		9
#define WLAN_FC_STYPE_DISASSOC		10
#define WLAN_FC_STYPE_AUTH		11
#define WLAN_FC_STYPE_DEAUTH		12
#define WLAN_FC_STYPE_ACTION		13

/* control */
#define WLAN_FC_STYPE_PSPOLL		10
#define WLAN_FC_STYPE_RTS		11
#define WLAN_FC_STYPE_CTS		12
#define WLAN_FC_STYPE_ACK		13
#define WLAN_FC_STYPE_CFEND		14
#define WLAN_FC_STYPE_CFENDACK		15

/* data */
#define WLAN_FC_STYPE_DATA		0
#define WLAN_FC_STYPE_DATA_CFACK	1
#define WLAN_FC_STYPE_DATA_CFPOLL	2
#define WLAN_FC_STYPE_DATA_CFACKPOLL	3
#define WLAN_FC_STYPE_NULLFUNC		4
#define WLAN_FC_STYPE_CFACK		5
#define WLAN_FC_STYPE_CFPOLL		6
#define WLAN_FC_STYPE_CFACKPOLL		7
#define WLAN_FC_STYPE_QOS_DATA		8
#define WLAN_FC_STYPE_QOS_DATA_CFACK	9
#define WLAN_FC_STYPE_QOS_DATA_CFPOLL	10
#define WLAN_FC_STYPE_QOS_DATA_CFACKPOLL	11
#define WLAN_FC_STYPE_QOS_NULL		12
#define WLAN_FC_STYPE_QOS_CFPOLL	14
#define WLAN_FC_STYPE_QOS_CFACKPOLL	15

/* Authentication algorithms */
#define WLAN_AUTH_OPEN			0
#define WLAN_AUTH_SHARED_KEY		1
#define WLAN_AUTH_FT			2
#define WLAN_AUTH_SAE			3
#define WLAN_AUTH_LEAP			128

#define WLAN_AUTH_CHALLENGE_LEN 128

#define WLAN_CAPABILITY_ESS BIT(0)
#define WLAN_CAPABILITY_IBSS BIT(1)
#define WLAN_CAPABILITY_CF_POLLABLE BIT(2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST BIT(3)
#define WLAN_CAPABILITY_PRIVACY BIT(4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE BIT(5)
#define WLAN_CAPABILITY_PBCC BIT(6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY BIT(7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT BIT(8)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME BIT(10)
#define WLAN_CAPABILITY_DSSS_OFDM BIT(13)

/* Status codes (IEEE 802.11-2007, 7.3.1.9, Table 7-23) */
#define WLAN_STATUS_SUCCESS 0
#define WLAN_STATUS_UNSPECIFIED_FAILURE 1
#define WLAN_STATUS_TDLS_WAKEUP_ALTERNATE 2
#define WLAN_STATUS_TDLS_WAKEUP_REJECT 3
#define WLAN_STATUS_SECURITY_DISABLED 5
#define WLAN_STATUS_UNACCEPTABLE_LIFETIME 6
#define WLAN_STATUS_NOT_IN_SAME_BSS 7
#define WLAN_STATUS_CAPS_UNSUPPORTED 10
#define WLAN_STATUS_REASSOC_NO_ASSOC 11
#define WLAN_STATUS_ASSOC_DENIED_UNSPEC 12
#define WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG 13
#define WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION 14
#define WLAN_STATUS_CHALLENGE_FAIL 15
#define WLAN_STATUS_AUTH_TIMEOUT 16
#define WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA 17
#define WLAN_STATUS_ASSOC_DENIED_RATES 18
/* IEEE 802.11b */
#define WLAN_STATUS_ASSOC_DENIED_NOSHORT 19
#define WLAN_STATUS_ASSOC_DENIED_NOPBCC 20
#define WLAN_STATUS_ASSOC_DENIED_NOAGILITY 21
/* IEEE 802.11h */
#define WLAN_STATUS_SPEC_MGMT_REQUIRED 22
#define WLAN_STATUS_PWR_CAPABILITY_NOT_VALID 23
#define WLAN_STATUS_SUPPORTED_CHANNEL_NOT_VALID 24
/* IEEE 802.11g */
#define WLAN_STATUS_ASSOC_DENIED_NO_SHORT_SLOT_TIME 25
#define WLAN_STATUS_ASSOC_DENIED_NO_DSSS_OFDM 26
#define WLAN_STATUS_ASSOC_DENIED_NO_HT 27
#define WLAN_STATUS_R0KH_UNREACHABLE 28
#define WLAN_STATUS_ASSOC_DENIED_NO_PCO 29
/* IEEE 802.11w */
#define WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY 30
#define WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION 31
#define WLAN_STATUS_UNSPECIFIED_QOS_FAILURE 32
#define WLAN_STATUS_REQUEST_DECLINED 37
#define WLAN_STATUS_INVALID_PARAMETERS 38
/* IEEE 802.11i */
#define WLAN_STATUS_INVALID_IE 40
#define WLAN_STATUS_GROUP_CIPHER_NOT_VALID 41
#define WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID 42
#define WLAN_STATUS_AKMP_NOT_VALID 43
#define WLAN_STATUS_UNSUPPORTED_RSN_IE_VERSION 44
#define WLAN_STATUS_INVALID_RSN_IE_CAPAB 45
#define WLAN_STATUS_CIPHER_REJECTED_PER_POLICY 46
#define WLAN_STATUS_TS_NOT_CREATED 47
#define WLAN_STATUS_DIRECT_LINK_NOT_ALLOWED 48
#define WLAN_STATUS_DEST_STA_NOT_PRESENT 49
#define WLAN_STATUS_DEST_STA_NOT_QOS_STA 50
#define WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE 51
/* IEEE 802.11r */
#define WLAN_STATUS_INVALID_FT_ACTION_FRAME_COUNT 52
#define WLAN_STATUS_INVALID_PMKID 53
#define WLAN_STATUS_INVALID_MDIE 54
#define WLAN_STATUS_INVALID_FTIE 55
#define WLAN_STATUS_GAS_ADV_PROTO_NOT_SUPPORTED 59
#define WLAN_STATUS_NO_OUTSTANDING_GAS_REQ 60
#define WLAN_STATUS_GAS_RESP_NOT_RECEIVED 61
#define WLAN_STATUS_STA_TIMED_OUT_WAITING_FOR_GAS_RESP 62
#define WLAN_STATUS_GAS_RESP_LARGER_THAN_LIMIT 63
#define WLAN_STATUS_REQ_REFUSED_HOME 64
#define WLAN_STATUS_ADV_SRV_UNREACHABLE 65
#define WLAN_STATUS_REQ_REFUSED_SSPN 67
#define WLAN_STATUS_REQ_REFUSED_UNAUTH_ACCESS 68
#define WLAN_STATUS_INVALID_RSNIE 72
#define WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ 76
#define WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED 77
#define WLAN_STATUS_TRANSMISSION_FAILURE 79
#define WLAN_STATUS_QUERY_RESP_OUTSTANDING 95
#define WLAN_STATUS_ASSOC_DENIED_NO_VHT 104

/* Reason codes (IEEE 802.11-2007, 7.3.1.7, Table 7-22) */
#define WLAN_REASON_UNSPECIFIED 1
#define WLAN_REASON_PREV_AUTH_NOT_VALID 2
#define WLAN_REASON_DEAUTH_LEAVING 3
#define WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY 4
#define WLAN_REASON_DISASSOC_AP_BUSY 5
#define WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA 6
#define WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA 7
#define WLAN_REASON_DISASSOC_STA_HAS_LEFT 8
#define WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH 9
/* IEEE 802.11h */
#define WLAN_REASON_PWR_CAPABILITY_NOT_VALID 10
#define WLAN_REASON_SUPPORTED_CHANNEL_NOT_VALID 11
/* IEEE 802.11i */
#define WLAN_REASON_INVALID_IE 13
#define WLAN_REASON_MICHAEL_MIC_FAILURE 14
#define WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT 15
#define WLAN_REASON_GROUP_KEY_UPDATE_TIMEOUT 16
#define WLAN_REASON_IE_IN_4WAY_DIFFERS 17
#define WLAN_REASON_GROUP_CIPHER_NOT_VALID 18
#define WLAN_REASON_PAIRWISE_CIPHER_NOT_VALID 19
#define WLAN_REASON_AKMP_NOT_VALID 20
#define WLAN_REASON_UNSUPPORTED_RSN_IE_VERSION 21
#define WLAN_REASON_INVALID_RSN_IE_CAPAB 22
#define WLAN_REASON_IEEE_802_1X_AUTH_FAILED 23
#define WLAN_REASON_CIPHER_SUITE_REJECTED 24
#define WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE 25
#define WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED 26
/* IEEE 802.11e */
#define WLAN_REASON_DISASSOC_LOW_ACK 34
/* IEEE 802.11s */
#define WLAN_REASON_MESH_PEERING_CANCELLED 52
#define WLAN_REASON_MESH_MAX_PEERS 53
#define WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION 54
#define WLAN_REASON_MESH_CLOSE_RCVD 55
#define WLAN_REASON_MESH_MAX_RETRIES 56
#define WLAN_REASON_MESH_CONFIRM_TIMEOUT 57
#define WLAN_REASON_MESH_INVALID_GTK 58
#define WLAN_REASON_MESH_INCONSISTENT_PARAMS 59
#define WLAN_REASON_MESH_INVALID_SECURITY_CAP 60


/* Information Element IDs */
#define WLAN_EID_SSID 0
#define WLAN_EID_SUPP_RATES 1
#define WLAN_EID_FH_PARAMS 2
#define WLAN_EID_DS_PARAMS 3
#define WLAN_EID_CF_PARAMS 4
#define WLAN_EID_TIM 5
#define WLAN_EID_IBSS_PARAMS 6
#define WLAN_EID_COUNTRY 7
#define WLAN_EID_BSS_LOAD 11
#define WLAN_EID_CHALLENGE 16
/* EIDs defined by IEEE 802.11h - START */
#define WLAN_EID_PWR_CONSTRAINT 32
#define WLAN_EID_PWR_CAPABILITY 33
#define WLAN_EID_TPC_REQUEST 34
#define WLAN_EID_TPC_REPORT 35
#define WLAN_EID_SUPPORTED_CHANNELS 36
#define WLAN_EID_CHANNEL_SWITCH 37
#define WLAN_EID_MEASURE_REQUEST 38
#define WLAN_EID_MEASURE_REPORT 39
#define WLAN_EID_QUITE 40
#define WLAN_EID_IBSS_DFS 41
/* EIDs defined by IEEE 802.11h - END */
#define WLAN_EID_ERP_INFO 42
#define WLAN_EID_HT_CAP 45
#define WLAN_EID_QOS 46
#define WLAN_EID_RSN 48
#define WLAN_EID_EXT_SUPP_RATES 50
#define WLAN_EID_NEIGHBOR_REPORT 52
#define WLAN_EID_MOBILITY_DOMAIN 54
#define WLAN_EID_FAST_BSS_TRANSITION 55
#define WLAN_EID_TIMEOUT_INTERVAL 56
#define WLAN_EID_RIC_DATA 57
#define WLAN_EID_SUPPORTED_OPERATING_CLASSES 59
#define WLAN_EID_HT_OPERATION 61
#define WLAN_EID_SECONDARY_CHANNEL_OFFSET 62
#define WLAN_EID_WAPI 68
#define WLAN_EID_TIME_ADVERTISEMENT 69
#define WLAN_EID_RRM_ENABLED_CAPABILITIES 70
#define WLAN_EID_20_40_BSS_COEXISTENCE 72
#define WLAN_EID_20_40_BSS_INTOLERANT 73
#define WLAN_EID_OVERLAPPING_BSS_SCAN_PARAMS 74
#define WLAN_EID_MMIE 76
#define WLAN_EID_SSID_LIST 84
#define WLAN_EID_BSS_MAX_IDLE_PERIOD 90
#define WLAN_EID_TFS_REQ 91
#define WLAN_EID_TFS_RESP 92
#define WLAN_EID_WNMSLEEP 93
#define WLAN_EID_TIME_ZONE 98
#define WLAN_EID_LINK_ID 101
#define WLAN_EID_INTERWORKING 107
#define WLAN_EID_ADV_PROTO 108
#define WLAN_EID_QOS_MAP_SET 110
#define WLAN_EID_ROAMING_CONSORTIUM 111
#define WLAN_EID_MESH_CONFIG 113
#define WLAN_EID_MESH_ID 114
#define WLAN_EID_PEER_MGMT 117
#define WLAN_EID_EXT_CAPAB 127
#define WLAN_EID_AMPE 139
#define WLAN_EID_MIC 140
#define WLAN_EID_CCKM 156
#define WLAN_EID_VHT_CAP 191
#define WLAN_EID_VHT_OPERATION 192
#define WLAN_EID_VHT_EXTENDED_BSS_LOAD 193
#define WLAN_EID_VHT_WIDE_BW_CHSWITCH  194
#define WLAN_EID_VHT_TRANSMIT_POWER_ENVELOPE 195
#define WLAN_EID_VHT_CHANNEL_SWITCH_WRAPPER 196
#define WLAN_EID_VHT_AID 197
#define WLAN_EID_VHT_QUIET_CHANNEL 198
#define WLAN_EID_VHT_OPERATING_MODE_NOTIFICATION 199
#define WLAN_EID_VENDOR_SPECIFIC 221


/* Action frame categories (IEEE 802.11-2007, 7.3.1.11, Table 7-24) */
#define WLAN_ACTION_SPECTRUM_MGMT 0
#define WLAN_ACTION_QOS 1
#define WLAN_ACTION_DLS 2
#define WLAN_ACTION_BLOCK_ACK 3
#define WLAN_ACTION_PUBLIC 4
#define WLAN_ACTION_RADIO_MEASUREMENT 5
#define WLAN_ACTION_FT 6
#define WLAN_ACTION_HT 7
#define WLAN_ACTION_SA_QUERY 8
#define WLAN_ACTION_PROTECTED_DUAL 9
#define WLAN_ACTION_WNM 10
#define WLAN_ACTION_UNPROTECTED_WNM 11
#define WLAN_ACTION_TDLS 12
#define WLAN_ACTION_SELF_PROTECTED 15
#define WLAN_ACTION_WMM 17 /* WMM Specification 1.1 */
#define WLAN_ACTION_VENDOR_SPECIFIC 127

/* Public action codes */
#define WLAN_PA_20_40_BSS_COEX 0
#define WLAN_PA_VENDOR_SPECIFIC 9
#define WLAN_PA_GAS_INITIAL_REQ 10
#define WLAN_PA_GAS_INITIAL_RESP 11
#define WLAN_PA_GAS_COMEBACK_REQ 12
#define WLAN_PA_GAS_COMEBACK_RESP 13
#define WLAN_TDLS_DISCOVERY_RESPONSE 14

/* Protected Dual of Public Action frames */
#define WLAN_PROT_DSE_ENABLEMENT 1
#define WLAN_PROT_DSE_DEENABLEMENT 2
#define WLAN_PROT_EXT_CSA 4
#define WLAN_PROT_MEASUREMENT_REQ 5
#define WLAN_PROT_MEASUREMENT_REPORT 6
#define WLAN_PROT_DSE_POWER_CONSTRAINT 8
#define WLAN_PROT_VENDOR_SPECIFIC 9
#define WLAN_PROT_GAS_INITIAL_REQ 10
#define WLAN_PROT_GAS_INITIAL_RESP 11
#define WLAN_PROT_GAS_COMEBACK_REQ 12
#define WLAN_PROT_GAS_COMEBACK_RESP 13

/* SA Query Action frame (IEEE 802.11w/D8.0, 7.4.9) */
#define WLAN_SA_QUERY_REQUEST 0
#define WLAN_SA_QUERY_RESPONSE 1

#define WLAN_SA_QUERY_TR_ID_LEN 2

/* TDLS action codes */
#define WLAN_TDLS_SETUP_REQUEST 0
#define WLAN_TDLS_SETUP_RESPONSE 1
#define WLAN_TDLS_SETUP_CONFIRM 2
#define WLAN_TDLS_TEARDOWN 3
#define WLAN_TDLS_PEER_TRAFFIC_INDICATION 4
#define WLAN_TDLS_CHANNEL_SWITCH_REQUEST 5
#define WLAN_TDLS_CHANNEL_SWITCH_RESPONSE 6
#define WLAN_TDLS_PEER_PSM_REQUEST 7
#define WLAN_TDLS_PEER_PSM_RESPONSE 8
#define WLAN_TDLS_PEER_TRAFFIC_RESPONSE 9
#define WLAN_TDLS_DISCOVERY_REQUEST 10

/* Radio Measurement Action codes */
#define WLAN_RRM_RADIO_MEASUREMENT_REQUEST 0
#define WLAN_RRM_RADIO_MEASUREMENT_REPORT 1
#define WLAN_RRM_LINK_MEASUREMENT_REQUEST 2
#define WLAN_RRM_LINK_MEASUREMENT_REPORT 3
#define WLAN_RRM_NEIGHBOR_REPORT_REQUEST 4
#define WLAN_RRM_NEIGHBOR_REPORT_RESPONSE 5

/* Radio Measurement capabilities (from RRM Capabilities IE) */
/* byte 1 (out of 5) */
#define WLAN_RRM_CAPS_LINK_MEASUREMENT BIT(0)
#define WLAN_RRM_CAPS_NEIGHBOR_REPORT BIT(1)

/* Timeout Interval Type */
#define WLAN_TIMEOUT_REASSOC_DEADLINE 1
#define WLAN_TIMEOUT_KEY_LIFETIME 2
#define WLAN_TIMEOUT_ASSOC_COMEBACK 3

/* Interworking element (IEEE 802.11u) - Access Network Options */
#define INTERWORKING_ANO_ACCESS_NETWORK_MASK 0x0f
#define INTERWORKING_ANO_INTERNET 0x10
#define INTERWORKING_ANO_ASRA 0x20
#define INTERWORKING_ANO_ESR 0x40
#define INTERWORKING_ANO_UESA 0x80

#define INTERWORKING_ANT_PRIVATE 0
#define INTERWORKING_ANT_PRIVATE_WITH_GUEST 1
#define INTERWORKING_ANT_CHARGEABLE_PUBLIC 2
#define INTERWORKING_ANT_FREE_PUBLIC 3
#define INTERWORKING_ANT_PERSONAL_DEVICE 4
#define INTERWORKING_ANT_EMERGENCY_SERVICES 5
#define INTERWORKING_ANT_TEST 6
#define INTERWORKING_ANT_WILDCARD 15

/* Advertisement Protocol ID definitions (IEEE Std 802.11u-2011) */
enum adv_proto_id {
	ACCESS_NETWORK_QUERY_PROTOCOL = 0,
	MIH_INFO_SERVICE = 1,
	MIH_CMD_AND_EVENT_DISCOVERY = 2,
	EMERGENCY_ALERT_SYSTEM = 3,
	ADV_PROTO_VENDOR_SPECIFIC = 221
};

/* Access Network Query Protocol info ID definitions (IEEE Std 802.11u-2011) */
enum anqp_info_id {
	ANQP_QUERY_LIST = 256,
	ANQP_CAPABILITY_LIST = 257,
	ANQP_VENUE_NAME = 258,
	ANQP_EMERGENCY_CALL_NUMBER = 259,
	ANQP_NETWORK_AUTH_TYPE = 260,
	ANQP_ROAMING_CONSORTIUM = 261,
	ANQP_IP_ADDR_TYPE_AVAILABILITY = 262,
	ANQP_NAI_REALM = 263,
	ANQP_3GPP_CELLULAR_NETWORK = 264,
	ANQP_AP_GEOSPATIAL_LOCATION = 265,
	ANQP_AP_CIVIC_LOCATION = 266,
	ANQP_AP_LOCATION_PUBLIC_URI = 267,
	ANQP_DOMAIN_NAME = 268,
	ANQP_EMERGENCY_ALERT_URI = 269,
	ANQP_EMERGENCY_NAI = 271,
	ANQP_VENDOR_SPECIFIC = 56797
};

/* NAI Realm list - EAP Method subfield - Authentication Parameter ID */
enum nai_realm_eap_auth_param {
	NAI_REALM_EAP_AUTH_EXPANDED_EAP_METHOD = 1,
	NAI_REALM_EAP_AUTH_NON_EAP_INNER_AUTH = 2,
	NAI_REALM_EAP_AUTH_INNER_AUTH_EAP_METHOD = 3,
	NAI_REALM_EAP_AUTH_EXPANDED_INNER_EAP_METHOD = 4,
	NAI_REALM_EAP_AUTH_CRED_TYPE = 5,
	NAI_REALM_EAP_AUTH_TUNNELED_CRED_TYPE = 6,
	NAI_REALM_EAP_AUTH_VENDOR_SPECIFIC = 221
};

enum nai_realm_eap_auth_inner_non_eap {
	NAI_REALM_INNER_NON_EAP_PAP = 1,
	NAI_REALM_INNER_NON_EAP_CHAP = 2,
	NAI_REALM_INNER_NON_EAP_MSCHAP = 3,
	NAI_REALM_INNER_NON_EAP_MSCHAPV2 = 4
};

enum nai_realm_eap_cred_type {
	NAI_REALM_CRED_TYPE_SIM = 1,
	NAI_REALM_CRED_TYPE_USIM = 2,
	NAI_REALM_CRED_TYPE_NFC_SECURE_ELEMENT = 3,
	NAI_REALM_CRED_TYPE_HARDWARE_TOKEN = 4,
	NAI_REALM_CRED_TYPE_SOFTOKEN = 5,
	NAI_REALM_CRED_TYPE_CERTIFICATE = 6,
	NAI_REALM_CRED_TYPE_USERNAME_PASSWORD = 7,
	NAI_REALM_CRED_TYPE_NONE = 8,
	NAI_REALM_CRED_TYPE_ANONYMOUS = 9,
	NAI_REALM_CRED_TYPE_VENDOR_SPECIFIC = 10
};

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

struct ieee80211_hdr {
	le16 frame_control;
	le16 duration_id;
	u8 addr1[6];
	u8 addr2[6];
	u8 addr3[6];
	le16 seq_ctrl;
	/* followed by 'u8 addr4[6];' if ToDS and FromDS is set in data frame
	 */
} STRUCT_PACKED;

#define IEEE80211_DA_FROMDS addr1
#define IEEE80211_BSSID_FROMDS addr2
#define IEEE80211_SA_FROMDS addr3

#define IEEE80211_HDRLEN (sizeof(struct ieee80211_hdr))

#define IEEE80211_FC(type, stype) host_to_le16((type << 2) | (stype << 4))

struct ieee80211_mgmt {
	le16 frame_control;
	le16 duration;
	u8 da[6];
	u8 sa[6];
	u8 bssid[6];
	le16 seq_ctrl;
	union {
		struct {
			le16 auth_alg;
			le16 auth_transaction;
			le16 status_code;
			/* possibly followed by Challenge text */
			u8 variable[0];
		} STRUCT_PACKED auth;
		struct {
			le16 reason_code;
			u8 variable[0];
		} STRUCT_PACKED deauth;
		struct {
			le16 capab_info;
			le16 listen_interval;
			/* followed by SSID and Supported rates */
			u8 variable[0];
		} STRUCT_PACKED assoc_req;
		struct {
			le16 capab_info;
			le16 status_code;
			le16 aid;
			/* followed by Supported rates */
			u8 variable[0];
		} STRUCT_PACKED assoc_resp, reassoc_resp;
		struct {
			le16 capab_info;
			le16 listen_interval;
			u8 current_ap[6];
			/* followed by SSID and Supported rates */
			u8 variable[0];
		} STRUCT_PACKED reassoc_req;
		struct {
			le16 reason_code;
			u8 variable[0];
		} STRUCT_PACKED disassoc;
		struct {
			u8 timestamp[8];
			le16 beacon_int;
			le16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params, TIM */
			u8 variable[0];
		} STRUCT_PACKED beacon;
		struct {
			/* only variable items: SSID, Supported rates */
			u8 variable[0];
		} STRUCT_PACKED probe_req;
		struct {
			u8 timestamp[8];
			le16 beacon_int;
			le16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params */
			u8 variable[0];
		} STRUCT_PACKED probe_resp;
		struct {
			u8 category;
			union {
				struct {
					u8 action_code;
					u8 dialog_token;
					u8 status_code;
					u8 variable[0];
				} STRUCT_PACKED wmm_action;
				struct{
					u8 action_code;
					u8 element_id;
					u8 length;
					u8 switch_mode;
					u8 new_chan;
					u8 switch_count;
				} STRUCT_PACKED chan_switch;
				struct {
					u8 action;
					u8 sta_addr[ETH_ALEN];
					u8 target_ap_addr[ETH_ALEN];
					u8 variable[0]; /* FT Request */
				} STRUCT_PACKED ft_action_req;
				struct {
					u8 action;
					u8 sta_addr[ETH_ALEN];
					u8 target_ap_addr[ETH_ALEN];
					le16 status_code;
					u8 variable[0]; /* FT Request */
				} STRUCT_PACKED ft_action_resp;
				struct {
					u8 action;
					u8 trans_id[WLAN_SA_QUERY_TR_ID_LEN];
				} STRUCT_PACKED sa_query_req;
				struct {
					u8 action; /* */
					u8 trans_id[WLAN_SA_QUERY_TR_ID_LEN];
				} STRUCT_PACKED sa_query_resp;
				struct {
					u8 action;
					u8 dialogtoken;
					u8 variable[0];
				} STRUCT_PACKED wnm_sleep_req;
				struct {
					u8 action;
					u8 dialogtoken;
					le16 keydata_len;
					u8 variable[0];
				} STRUCT_PACKED wnm_sleep_resp;
				struct {
					u8 action;
					u8 variable[0];
				} STRUCT_PACKED public_action;
				struct {
					u8 action; /* 9 */
					u8 oui[3];
					/* Vendor-specific content */
					u8 variable[0];
				} STRUCT_PACKED vs_public_action;
				struct {
					u8 action; /* 7 */
					u8 dialog_token;
					u8 req_mode;
					le16 disassoc_timer;
					u8 validity_interval;
					/* BSS Termination Duration (optional),
					 * Session Information URL (optional),
					 * BSS Transition Candidate List
					 * Entries */
					u8 variable[0];
				} STRUCT_PACKED bss_tm_req;
				struct {
					u8 action; /* 8 */
					u8 dialog_token;
					u8 status_code;
					u8 bss_termination_delay;
					/* Target BSSID (optional),
					 * BSS Transition Candidate List
					 * Entries (optional) */
					u8 variable[0];
				} STRUCT_PACKED bss_tm_resp;
				struct {
					u8 action; /* 6 */
					u8 dialog_token;
					u8 query_reason;
					/* BSS Transition Candidate List
					 * Entries (optional) */
					u8 variable[0];
				} STRUCT_PACKED bss_tm_query;
				struct {
					u8 action; /* 15 */
					u8 variable[0];
				} STRUCT_PACKED slf_prot_action;
			} u;
		} STRUCT_PACKED action;
	} u;
} STRUCT_PACKED;


/* Rx MCS bitmask is in the first 77 bits of supported_mcs_set */
#define IEEE80211_HT_MCS_MASK_LEN 10

/* HT Capabilities element */
struct ieee80211_ht_capabilities {
	le16 ht_capabilities_info;
	u8 a_mpdu_params; /* Maximum A-MPDU Length Exponent B0..B1
			   * Minimum MPDU Start Spacing B2..B4
			   * Reserved B5..B7 */
	u8 supported_mcs_set[16];
	le16 ht_extended_capabilities;
	le32 tx_bf_capability_info;
	u8 asel_capabilities;
} STRUCT_PACKED;


/* HT Operation element */
struct ieee80211_ht_operation {
	u8 primary_chan;
	/* Five octets of HT Operation Information */
	u8 ht_param; /* B0..B7 */
	le16 operation_mode; /* B8..B23 */
	le16 param; /* B24..B39 */
	u8 basic_mcs_set[16];
} STRUCT_PACKED;


struct ieee80211_obss_scan_parameters {
	le16 scan_passive_dwell;
	le16 scan_active_dwell;
	le16 width_trigger_scan_interval;
	le16 scan_passive_total_per_channel;
	le16 scan_active_total_per_channel;
	le16 channel_transition_delay_factor;
	le16 scan_activity_threshold;
} STRUCT_PACKED;


struct ieee80211_vht_capabilities {
	le32 vht_capabilities_info;
	struct {
		le16 rx_map;
		le16 rx_highest;
		le16 tx_map;
		le16 tx_highest;
	} vht_supported_mcs_set;
} STRUCT_PACKED;

struct ieee80211_vht_operation {
	u8 vht_op_info_chwidth;
	u8 vht_op_info_chan_center_freq_seg0_idx;
	u8 vht_op_info_chan_center_freq_seg1_idx;
	le16 vht_basic_mcs_set;
} STRUCT_PACKED;

struct ieee80211_ampe_ie {
	u8 selected_pairwise_suite[4];
	u8 local_nonce[32];
	u8 peer_nonce[32];
	u8 mgtk[16];
	u8 key_rsc[8];
	u8 key_expiration[4];
} STRUCT_PACKED;

#ifdef _MSC_VER
#pragma pack(pop)
#endif /* _MSC_VER */

#define ERP_INFO_NON_ERP_PRESENT BIT(0)
#define ERP_INFO_USE_PROTECTION BIT(1)
#define ERP_INFO_BARKER_PREAMBLE_MODE BIT(2)

#define OVERLAPPING_BSS_TRANS_DELAY_FACTOR 5

/* HT Capabilities Info field within HT Capabilities element */
#define HT_CAP_INFO_LDPC_CODING_CAP		((u16) BIT(0))
#define HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET	((u16) BIT(1))
#define HT_CAP_INFO_SMPS_MASK			((u16) (BIT(2) | BIT(3)))
#define HT_CAP_INFO_SMPS_STATIC			((u16) 0)
#define HT_CAP_INFO_SMPS_DYNAMIC		((u16) BIT(2))
#define HT_CAP_INFO_SMPS_DISABLED		((u16) (BIT(2) | BIT(3)))
#define HT_CAP_INFO_GREEN_FIELD			((u16) BIT(4))
#define HT_CAP_INFO_SHORT_GI20MHZ		((u16) BIT(5))
#define HT_CAP_INFO_SHORT_GI40MHZ		((u16) BIT(6))
#define HT_CAP_INFO_TX_STBC			((u16) BIT(7))
#define HT_CAP_INFO_RX_STBC_MASK		((u16) (BIT(8) | BIT(9)))
#define HT_CAP_INFO_RX_STBC_1			((u16) BIT(8))
#define HT_CAP_INFO_RX_STBC_12			((u16) BIT(9))
#define HT_CAP_INFO_RX_STBC_123			((u16) (BIT(8) | BIT(9)))
#define HT_CAP_INFO_DELAYED_BA			((u16) BIT(10))
#define HT_CAP_INFO_MAX_AMSDU_SIZE		((u16) BIT(11))
#define HT_CAP_INFO_DSSS_CCK40MHZ		((u16) BIT(12))
/* B13 - Reserved (was PSMP support during P802.11n development) */
#define HT_CAP_INFO_40MHZ_INTOLERANT		((u16) BIT(14))
#define HT_CAP_INFO_LSIG_TXOP_PROTECT_SUPPORT	((u16) BIT(15))

/* HT Extended Capabilities field within HT Capabilities element */
#define EXT_HT_CAP_INFO_PCO			((u16) BIT(0))
#define EXT_HT_CAP_INFO_PCO_TRANS_TIME_MASK	((u16) (BIT(1) | BIT(2)))
#define EXT_HT_CAP_INFO_TRANS_TIME_OFFSET	1
/* B3..B7 - Reserved */
#define EXT_HT_CAP_INFO_MCS_FEEDBACK_MASK	((u16) (BIT(8) | BIT(9)))
#define EXT_HT_CAP_INFO_MCS_FEEDBACK_OFFSET	8
#define EXT_HT_CAP_INFO_HTC_SUPPORT		((u16) BIT(10))
#define EXT_HT_CAP_INFO_RD_RESPONDER		((u16) BIT(11))
/* B12..B15 - Reserved */

/* Transmit Beanforming Capabilities within HT Capabilities element */
#define TX_BF_CAP_IMPLICIT_TXBF_RX_CAP ((u32) BIT(0))
#define TX_BF_CAP_RX_STAGGERED_SOUNDING_CAP ((u32) BIT(1))
#define TX_BF_CAP_TX_STAGGERED_SOUNDING_CAP ((u32) BIT(2))
#define TX_BF_CAP_RX_NDP_CAP ((u32) BIT(3))
#define TX_BF_CAP_TX_NDP_CAP ((u32) BIT(4))
#define TX_BF_CAP_IMPLICIT_TX_BF_CAP ((u32) BIT(5))
#define TX_BF_CAP_CALIBRATION_MASK ((u32) (BIT(6) | BIT(7))
#define TX_BF_CAP_CALIB_OFFSET 6
#define TX_BF_CAP_EXPLICIT_CSI_TXBF_CAP ((u32) BIT(8))
#define TX_BF_CAP_EXPLICIT_NONCOMPR_STEERING_CAP ((u32) BIT(9))
#define TX_BF_CAP_EXPLICIT_COMPR_STEERING_CAP ((u32) BIT(10))
#define TX_BF_CAP_EXPLICIT_TX_BF_CSI_FEEDBACK_MASK ((u32) (BIT(10) | BIT(11)))
#define TX_BF_CAP_EXPLICIT_BF_CSI_FEEDBACK_OFFSET 11
#define TX_BF_CAP_EXPLICIT_UNCOMPR_STEERING_MATRIX_FEEDBACK_OFFSET 13
#define TX_BF_CAP_EXPLICIT_COMPRESSED_STEERING_MATRIX_FEEDBACK_OFFSET 15
#define TX_BF_CAP_MINIMAL_GROUPING_OFFSET 17
#define TX_BF_CAP_CSI_NUM_BEAMFORMER_ANT_OFFSET 19
#define TX_BF_CAP_UNCOMPRESSED_STEERING_MATRIX_BEAMFORMER_ANT_OFFSET 21
#define TX_BF_CAP_COMPRESSED_STEERING_MATRIX_BEAMFORMER_ANT_OFFSET 23
#define TX_BF_CAP_SCI_MAX_OF_ROWS_BEANFORMER_SUPPORTED_OFFSET 25
#define TX_BF_CAP_CHANNEL_ESTIMATION_CAP_MASK ((u32) (BIT(27) | BIT(28)))
#define TX_BF_CAP_CHANNEL_ESTIMATION_CAP_OFFSET 27
/* B29..B31 - Reserved */

/* ASEL Capability field within HT Capabilities element */
#define ASEL_CAP_ASEL_CAPABLE ((u8) BIT(0))
#define ASEL_CAP_EXPLICIT_CSI_FEEDBACK_BASED_TX_AS_CAP ((u8) BIT(1))
#define ASEL_CAP_ANT_INDICES_FEEDBACK_BASED_TX_AS_CAP ((u8) BIT(2))
#define ASEL_CAP_EXPLICIT_CSI_FEEDBACK_CAP ((u8) BIT(3))
#define ASEL_CAP_ANT_INDICES_FEEDBACK_CAP ((u8) BIT(4))
#define ASEL_CAP_RX_AS_CAP ((u8) BIT(5))
#define ASEL_CAP_TX_SOUNDING_PPDUS_CAP ((u8) BIT(6))
/* B7 - Reserved */

/* First octet of HT Operation Information within HT Operation element */
#define HT_INFO_HT_PARAM_SECONDARY_CHNL_OFF_MASK	((u8) BIT(0) | BIT(1))
#define HT_INFO_HT_PARAM_SECONDARY_CHNL_ABOVE		((u8) BIT(0))
#define HT_INFO_HT_PARAM_SECONDARY_CHNL_BELOW		((u8) BIT(0) | BIT(1))
#define HT_INFO_HT_PARAM_STA_CHNL_WIDTH			((u8) BIT(2))
#define HT_INFO_HT_PARAM_RIFS_MODE			((u8) BIT(3))
/* B4..B7 - Reserved */

/* HT Protection (B8..B9 of HT Operation Information) */
#define HT_PROT_NO_PROTECTION           0
#define HT_PROT_NONMEMBER_PROTECTION    1
#define HT_PROT_20MHZ_PROTECTION        2
#define HT_PROT_NON_HT_MIXED            3
/* Bits within ieee80211_ht_operation::operation_mode (BIT(0) maps to B8 in
 * HT Operation Information) */
#define HT_OPER_OP_MODE_HT_PROT_MASK ((u16) (BIT(0) | BIT(1))) /* B8..B9 */
#define HT_OPER_OP_MODE_NON_GF_HT_STAS_PRESENT	((u16) BIT(2)) /* B10 */
/* BIT(3), i.e., B11 in HT Operation Information field - Reserved */
#define HT_OPER_OP_MODE_OBSS_NON_HT_STAS_PRESENT	((u16) BIT(4)) /* B12 */
/* BIT(5)..BIT(15), i.e., B13..B23 - Reserved */

/* Last two octets of HT Operation Information (BIT(0) = B24) */
/* B24..B29 - Reserved */
#define HT_OPER_PARAM_DUAL_BEACON			((u16) BIT(6))
#define HT_OPER_PARAM_DUAL_CTS_PROTECTION		((u16) BIT(7))
#define HT_OPER_PARAM_STBC_BEACON			((u16) BIT(8))
#define HT_OPER_PARAM_LSIG_TXOP_PROT_FULL_SUPP		((u16) BIT(9))
#define HT_OPER_PARAM_PCO_ACTIVE			((u16) BIT(10))
#define HT_OPER_PARAM_PCO_PHASE				((u16) BIT(11))
/* B36..B39 - Reserved */

#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127

/* VHT Defines */
#define VHT_CAP_MAX_MPDU_LENGTH_7991                ((u32) BIT(0))
#define VHT_CAP_MAX_MPDU_LENGTH_11454               ((u32) BIT(1))
#define VHT_CAP_MAX_MPDU_LENGTH_MASK                ((u32) BIT(0) | BIT(1))
#define VHT_CAP_MAX_MPDU_LENGTH_MASK_SHIFT          0
#define VHT_CAP_SUPP_CHAN_WIDTH_160MHZ              ((u32) BIT(2))
#define VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ     ((u32) BIT(3))
#define VHT_CAP_SUPP_CHAN_WIDTH_MASK                ((u32) BIT(2) | BIT(3))
#define VHT_CAP_RXLDPC                              ((u32) BIT(4))
#define VHT_CAP_SHORT_GI_80                         ((u32) BIT(5))
#define VHT_CAP_SHORT_GI_160                        ((u32) BIT(6))
#define VHT_CAP_TXSTBC                              ((u32) BIT(7))
#define VHT_CAP_RXSTBC_1                            ((u32) BIT(8))
#define VHT_CAP_RXSTBC_2                            ((u32) BIT(9))
#define VHT_CAP_RXSTBC_3                            ((u32) BIT(8) | BIT(9))
#define VHT_CAP_RXSTBC_4                            ((u32) BIT(10))
#define VHT_CAP_RXSTBC_MASK                         ((u32) BIT(8) | BIT(9) | \
							   BIT(10))
#define VHT_CAP_RXSTBC_MASK_SHIFT                   8
#define VHT_CAP_SU_BEAMFORMER_CAPABLE               ((u32) BIT(11))
#define VHT_CAP_SU_BEAMFORMEE_CAPABLE               ((u32) BIT(12))
#define VHT_CAP_BEAMFORMEE_STS_MAX                  ((u32) BIT(13) | \
							   BIT(14) | BIT(15))
#define VHT_CAP_BEAMFORMEE_STS_MAX_SHIFT            13
#define VHT_CAP_BEAMFORMEE_STS_OFFSET               13
#define VHT_CAP_SOUNDING_DIMENSION_MAX              ((u32) BIT(16) | \
							   BIT(17) | BIT(18))
#define VHT_CAP_SOUNDING_DIMENSION_MAX_SHIFT        16
#define VHT_CAP_SOUNDING_DIMENSION_OFFSET           16
#define VHT_CAP_MU_BEAMFORMER_CAPABLE               ((u32) BIT(19))
#define VHT_CAP_MU_BEAMFORMEE_CAPABLE               ((u32) BIT(20))
#define VHT_CAP_VHT_TXOP_PS                         ((u32) BIT(21))
#define VHT_CAP_HTC_VHT                             ((u32) BIT(22))

#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_1        ((u32) BIT(23))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_2        ((u32) BIT(24))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_3        ((u32) BIT(23) | BIT(24))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_4        ((u32) BIT(25))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_5        ((u32) BIT(23) | BIT(25))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_6        ((u32) BIT(24) | BIT(25))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX      ((u32) BIT(23) | \
							   BIT(24) | BIT(25))
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX_SHIFT 23
#define VHT_CAP_VHT_LINK_ADAPTATION_VHT_UNSOL_MFB   ((u32) BIT(27))
#define VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB     ((u32) BIT(26) | BIT(27))
#define VHT_CAP_RX_ANTENNA_PATTERN                  ((u32) BIT(28))
#define VHT_CAP_TX_ANTENNA_PATTERN                  ((u32) BIT(29))

#define VHT_OPMODE_CHANNEL_WIDTH_MASK		    ((u8) BIT(0) | BIT(1))
#define VHT_OPMODE_CHANNEL_RxNSS_MASK		    ((u8) BIT(4) | BIT(5) | \
						     BIT(6))
#define VHT_OPMODE_NOTIF_RX_NSS_SHIFT		    4

#define VHT_RX_NSS_MAX_STREAMS			    8

/* VHT channel widths */
#define VHT_CHANWIDTH_USE_HT	0
#define VHT_CHANWIDTH_80MHZ	1
#define VHT_CHANWIDTH_160MHZ	2
#define VHT_CHANWIDTH_80P80MHZ	3

#define OUI_MICROSOFT 0x0050f2 /* Microsoft (also used in Wi-Fi specs)
				* 00:50:F2 */
#define WPA_IE_VENDOR_TYPE 0x0050f201
#define WMM_IE_VENDOR_TYPE 0x0050f202
#define WPS_IE_VENDOR_TYPE 0x0050f204
#define OUI_WFA 0x506f9a
#define P2P_IE_VENDOR_TYPE 0x506f9a09
#define WFD_IE_VENDOR_TYPE 0x506f9a0a
#define WFD_OUI_TYPE 10
#define HS20_IE_VENDOR_TYPE 0x506f9a10
#define OSEN_IE_VENDOR_TYPE 0x506f9a12

#define WMM_OUI_TYPE 2
#define WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define WMM_OUI_SUBTYPE_PARAMETER_ELEMENT 1
#define WMM_OUI_SUBTYPE_TSPEC_ELEMENT 2
#define WMM_VERSION 1

#define WMM_ACTION_CODE_ADDTS_REQ 0
#define WMM_ACTION_CODE_ADDTS_RESP 1
#define WMM_ACTION_CODE_DELTS 2

#define WMM_ADDTS_STATUS_ADMISSION_ACCEPTED 0
#define WMM_ADDTS_STATUS_INVALID_PARAMETERS 1
/* 2 - Reserved */
#define WMM_ADDTS_STATUS_REFUSED 3
/* 4-255 - Reserved */

/* WMM TSPEC Direction Field Values */
#define WMM_TSPEC_DIRECTION_UPLINK 0
#define WMM_TSPEC_DIRECTION_DOWNLINK 1
/* 2 - Reserved */
#define WMM_TSPEC_DIRECTION_BI_DIRECTIONAL 3

/*
 * WMM Information Element (used in (Re)Association Request frames; may also be
 * used in Beacon frames)
 */
struct wmm_information_element {
	/* Element ID: 221 (0xdd); Length: 7 */
	/* required fields for WMM version 1 */
	u8 oui[3]; /* 00:50:f2 */
	u8 oui_type; /* 2 */
	u8 oui_subtype; /* 0 */
	u8 version; /* 1 for WMM version 1.0 */
	u8 qos_info; /* AP/STA specific QoS info */

} STRUCT_PACKED;

#define WMM_QOSINFO_AP_UAPSD 0x80

#define WMM_QOSINFO_STA_AC_MASK 0x0f
#define WMM_QOSINFO_STA_SP_MASK 0x03
#define WMM_QOSINFO_STA_SP_SHIFT 5

#define WMM_AC_AIFSN_MASK 0x0f
#define WMM_AC_AIFNS_SHIFT 0
#define WMM_AC_ACM 0x10
#define WMM_AC_ACI_MASK 0x60
#define WMM_AC_ACI_SHIFT 5

#define WMM_AC_ECWMIN_MASK 0x0f
#define WMM_AC_ECWMIN_SHIFT 0
#define WMM_AC_ECWMAX_MASK 0xf0
#define WMM_AC_ECWMAX_SHIFT 4

struct wmm_ac_parameter {
	u8 aci_aifsn; /* AIFSN, ACM, ACI */
	u8 cw; /* ECWmin, ECWmax (CW = 2^ECW - 1) */
	le16 txop_limit;
}  STRUCT_PACKED;

/*
 * WMM Parameter Element (used in Beacon, Probe Response, and (Re)Association
 * Response frmaes)
 */
struct wmm_parameter_element {
	/* Element ID: 221 (0xdd); Length: 24 */
	/* required fields for WMM version 1 */
	u8 oui[3]; /* 00:50:f2 */
	u8 oui_type; /* 2 */
	u8 oui_subtype; /* 1 */
	u8 version; /* 1 for WMM version 1.0 */
	u8 qos_info; /* AP/STA specific QoS info */
	u8 reserved; /* 0 */
	struct wmm_ac_parameter ac[4]; /* AC_BE, AC_BK, AC_VI, AC_VO */

} STRUCT_PACKED;

/* WMM TSPEC Element */
struct wmm_tspec_element {
	u8 eid; /* 221 = 0xdd */
	u8 length; /* 6 + 55 = 61 */
	u8 oui[3]; /* 00:50:f2 */
	u8 oui_type; /* 2 */
	u8 oui_subtype; /* 2 */
	u8 version; /* 1 */
	/* WMM TSPEC body (55 octets): */
	u8 ts_info[3];
	le16 nominal_msdu_size;
	le16 maximum_msdu_size;
	le32 minimum_service_interval;
	le32 maximum_service_interval;
	le32 inactivity_interval;
	le32 suspension_interval;
	le32 service_start_time;
	le32 minimum_data_rate;
	le32 mean_data_rate;
	le32 peak_data_rate;
	le32 maximum_burst_size;
	le32 delay_bound;
	le32 minimum_phy_rate;
	le16 surplus_bandwidth_allowance;
	le16 medium_time;
} STRUCT_PACKED;


/* Access Categories / ACI to AC coding */
enum wmm_ac {
	WMM_AC_BE = 0 /* Best Effort */,
	WMM_AC_BK = 1 /* Background */,
	WMM_AC_VI = 2 /* Video */,
	WMM_AC_VO = 3 /* Voice */,
	WMM_AC_NUM = 4
};


#define HS20_INDICATION_OUI_TYPE 16
#define HS20_ANQP_OUI_TYPE 17
#define HS20_OSEN_OUI_TYPE 18
#define HS20_STYPE_QUERY_LIST 1
#define HS20_STYPE_CAPABILITY_LIST 2
#define HS20_STYPE_OPERATOR_FRIENDLY_NAME 3
#define HS20_STYPE_WAN_METRICS 4
#define HS20_STYPE_CONNECTION_CAPABILITY 5
#define HS20_STYPE_NAI_HOME_REALM_QUERY 6
#define HS20_STYPE_OPERATING_CLASS 7
#define HS20_STYPE_OSU_PROVIDERS_LIST 8
#define HS20_STYPE_ICON_REQUEST 10
#define HS20_STYPE_ICON_BINARY_FILE 11

#define HS20_DGAF_DISABLED 0x01
#define HS20_PPS_MO_ID_PRESENT 0x02
#define HS20_ANQP_DOMAIN_ID_PRESENT 0x04
#define HS20_VERSION 0x10 /* Release 2 */

/* WNM-Notification WFA vendors specific subtypes */
#define HS20_WNM_SUB_REM_NEEDED 0
#define HS20_WNM_DEAUTH_IMMINENT_NOTICE 1

#define HS20_DEAUTH_REASON_CODE_BSS 0
#define HS20_DEAUTH_REASON_CODE_ESS 1

/* Wi-Fi Direct (P2P) */

#define P2P_OUI_TYPE 9

enum p2p_attr_id {
	P2P_ATTR_STATUS = 0,
	P2P_ATTR_MINOR_REASON_CODE = 1,
	P2P_ATTR_CAPABILITY = 2,
	P2P_ATTR_DEVICE_ID = 3,
	P2P_ATTR_GROUP_OWNER_INTENT = 4,
	P2P_ATTR_CONFIGURATION_TIMEOUT = 5,
	P2P_ATTR_LISTEN_CHANNEL = 6,
	P2P_ATTR_GROUP_BSSID = 7,
	P2P_ATTR_EXT_LISTEN_TIMING = 8,
	P2P_ATTR_INTENDED_INTERFACE_ADDR = 9,
	P2P_ATTR_MANAGEABILITY = 10,
	P2P_ATTR_CHANNEL_LIST = 11,
	P2P_ATTR_NOTICE_OF_ABSENCE = 12,
	P2P_ATTR_DEVICE_INFO = 13,
	P2P_ATTR_GROUP_INFO = 14,
	P2P_ATTR_GROUP_ID = 15,
	P2P_ATTR_INTERFACE = 16,
	P2P_ATTR_OPERATING_CHANNEL = 17,
	P2P_ATTR_INVITATION_FLAGS = 18,
	P2P_ATTR_OOB_GO_NEG_CHANNEL = 19,
	P2P_ATTR_SERVICE_HASH = 21,
	P2P_ATTR_SESSION_INFORMATION_DATA = 22,
	P2P_ATTR_CONNECTION_CAPABILITY = 23,
	P2P_ATTR_ADVERTISEMENT_ID = 24,
	P2P_ATTR_ADVERTISED_SERVICE = 25,
	P2P_ATTR_SESSION_ID = 26,
	P2P_ATTR_FEATURE_CAPABILITY = 27,
	P2P_ATTR_PERSISTENT_GROUP = 28,
	P2P_ATTR_VENDOR_SPECIFIC = 221
};

#define P2P_MAX_GO_INTENT 15

/* P2P Capability - Device Capability bitmap */
#define P2P_DEV_CAPAB_SERVICE_DISCOVERY BIT(0)
#define P2P_DEV_CAPAB_CLIENT_DISCOVERABILITY BIT(1)
#define P2P_DEV_CAPAB_CONCURRENT_OPER BIT(2)
#define P2P_DEV_CAPAB_INFRA_MANAGED BIT(3)
#define P2P_DEV_CAPAB_DEVICE_LIMIT BIT(4)
#define P2P_DEV_CAPAB_INVITATION_PROCEDURE BIT(5)

/* P2P Capability - Group Capability bitmap */
#define P2P_GROUP_CAPAB_GROUP_OWNER BIT(0)
#define P2P_GROUP_CAPAB_PERSISTENT_GROUP BIT(1)
#define P2P_GROUP_CAPAB_GROUP_LIMIT BIT(2)
#define P2P_GROUP_CAPAB_INTRA_BSS_DIST BIT(3)
#define P2P_GROUP_CAPAB_CROSS_CONN BIT(4)
#define P2P_GROUP_CAPAB_PERSISTENT_RECONN BIT(5)
#define P2P_GROUP_CAPAB_GROUP_FORMATION BIT(6)
#define P2P_GROUP_CAPAB_IP_ADDR_ALLOCATION BIT(7)

/* Invitation Flags */
#define P2P_INVITATION_FLAGS_TYPE BIT(0)

/* P2P Manageability */
#define P2P_MAN_DEVICE_MANAGEMENT BIT(0)
#define P2P_MAN_CROSS_CONNECTION_PERMITTED BIT(1)
#define P2P_MAN_COEXISTENCE_OPTIONAL BIT(2)

enum p2p_status_code {
	P2P_SC_SUCCESS = 0,
	P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE = 1,
	P2P_SC_FAIL_INCOMPATIBLE_PARAMS = 2,
	P2P_SC_FAIL_LIMIT_REACHED = 3,
	P2P_SC_FAIL_INVALID_PARAMS = 4,
	P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE = 5,
	P2P_SC_FAIL_PREV_PROTOCOL_ERROR = 6,
	P2P_SC_FAIL_NO_COMMON_CHANNELS = 7,
	P2P_SC_FAIL_UNKNOWN_GROUP = 8,
	P2P_SC_FAIL_BOTH_GO_INTENT_15 = 9,
	P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD = 10,
	P2P_SC_FAIL_REJECTED_BY_USER = 11,
	P2P_SC_SUCCESS_DEFERRED = 12,
};

enum p2p_role_indication {
	P2P_DEVICE_NOT_IN_GROUP = 0x00,
	P2P_CLIENT_IN_A_GROUP = 0x01,
	P2P_GO_IN_A_GROUP = 0x02,
};

#define P2P_WILDCARD_SSID "DIRECT-"
#define P2P_WILDCARD_SSID_LEN 7

/* P2P action frames */
enum p2p_act_frame_type {
	P2P_NOA = 0,
	P2P_PRESENCE_REQ = 1,
	P2P_PRESENCE_RESP = 2,
	P2P_GO_DISC_REQ = 3
};

/* P2P public action frames */
enum p2p_action_frame_type {
	P2P_GO_NEG_REQ = 0,
	P2P_GO_NEG_RESP = 1,
	P2P_GO_NEG_CONF = 2,
	P2P_INVITATION_REQ = 3,
	P2P_INVITATION_RESP = 4,
	P2P_DEV_DISC_REQ = 5,
	P2P_DEV_DISC_RESP = 6,
	P2P_PROV_DISC_REQ = 7,
	P2P_PROV_DISC_RESP = 8
};

enum p2p_service_protocol_type {
	P2P_SERV_ALL_SERVICES = 0,
	P2P_SERV_BONJOUR = 1,
	P2P_SERV_UPNP = 2,
	P2P_SERV_WS_DISCOVERY = 3,
	P2P_SERV_WIFI_DISPLAY = 4,
	P2P_SERV_P2PS = 11,
	P2P_SERV_VENDOR_SPECIFIC = 255
};

enum p2p_sd_status {
	P2P_SD_SUCCESS = 0,
	P2P_SD_PROTO_NOT_AVAILABLE = 1,
	P2P_SD_REQUESTED_INFO_NOT_AVAILABLE = 2,
	P2P_SD_BAD_REQUEST = 3
};


enum wifi_display_subelem {
	WFD_SUBELEM_DEVICE_INFO = 0,
	WFD_SUBELEM_ASSOCIATED_BSSID = 1,
	WFD_SUBELEM_AUDIO_FORMATS = 2,
	WFD_SUBELEM_VIDEO_FORMATS = 3,
	WFD_SUBELEM_3D_VIDEO_FORMATS = 4,
	WFD_SUBELEM_CONTENT_PROTECTION = 5,
	WFD_SUBELEM_COUPLED_SINK = 6,
	WFD_SUBELEM_EXT_CAPAB = 7,
	WFD_SUBELEM_LOCAL_IP_ADDRESS = 8,
	WFD_SUBELEM_SESSION_INFO = 9
};

/* 802.11s */
#define MESH_SYNC_METHOD_NEIGHBOR_OFFSET 1
#define MESH_SYNC_METHOD_VENDOR		255
#define MESH_PATH_PROTOCOL_HWMP		1
#define MESH_PATH_PROTOCOL_VENDOR	255
#define MESH_PATH_METRIC_AIRTIME	1
#define MESH_PATH_METRIC_VENDOR		255

enum plink_action_field {
	PLINK_OPEN = 1,
	PLINK_CONFIRM,
	PLINK_CLOSE
};

#define OUI_BROADCOM 0x00904c /* Broadcom (Epigram) */
#define VENDOR_VHT_TYPE		0x04
#define VENDOR_VHT_SUBTYPE	0x08
#define VENDOR_VHT_SUBTYPE2	0x00

#define VENDOR_HT_CAPAB_OUI_TYPE 0x33 /* 00-90-4c:0x33 */

/* cipher suite selectors */
#define WLAN_CIPHER_SUITE_USE_GROUP	0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40		0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP		0x000FAC02
/* reserved: 				0x000FAC03 */
#define WLAN_CIPHER_SUITE_CCMP		0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104	0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC	0x000FAC06
#define WLAN_CIPHER_SUITE_NO_GROUP_ADDR	0x000FAC07
#define WLAN_CIPHER_SUITE_GCMP		0x000FAC08
#define WLAN_CIPHER_SUITE_GCMP_256	0x000FAC09
#define WLAN_CIPHER_SUITE_CCMP_256	0x000FAC0A
#define WLAN_CIPHER_SUITE_BIP_GMAC_128	0x000FAC0B
#define WLAN_CIPHER_SUITE_BIP_GMAC_256	0x000FAC0C
#define WLAN_CIPHER_SUITE_BIP_CMAC_256	0x000FAC0D

#define WLAN_CIPHER_SUITE_SMS4		0x00147201

#define WLAN_CIPHER_SUITE_CKIP		0x00409600
#define WLAN_CIPHER_SUITE_CKIP_CMIC	0x00409601
#define WLAN_CIPHER_SUITE_CMIC		0x00409602
#define WLAN_CIPHER_SUITE_KRK		0x004096FF /* for nl80211 use only */

/* AKM suite selectors */
#define WLAN_AKM_SUITE_8021X		0x000FAC01
#define WLAN_AKM_SUITE_PSK		0x000FAC02
#define WLAN_AKM_SUITE_FT_8021X		0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK		0x000FAC04
#define WLAN_AKM_SUITE_8021X_SHA256	0x000FAC05
#define WLAN_AKM_SUITE_PSK_SHA256	0x000FAC06
#define WLAN_AKM_SUITE_8021X_SUITE_B	0x000FAC11
#define WLAN_AKM_SUITE_8021X_SUITE_B_192	0x000FAC12
#define WLAN_AKM_SUITE_CCKM		0x00409600
#define WLAN_AKM_SUITE_OSEN		0x506f9a01


/* IEEE 802.11v - WNM Action field values */
enum wnm_action {
	WNM_EVENT_REQ = 0,
	WNM_EVENT_REPORT = 1,
	WNM_DIAGNOSTIC_REQ = 2,
	WNM_DIAGNOSTIC_REPORT = 3,
	WNM_LOCATION_CFG_REQ = 4,
	WNM_LOCATION_CFG_RESP = 5,
	WNM_BSS_TRANS_MGMT_QUERY = 6,
	WNM_BSS_TRANS_MGMT_REQ = 7,
	WNM_BSS_TRANS_MGMT_RESP = 8,
	WNM_FMS_REQ = 9,
	WNM_FMS_RESP = 10,
	WNM_COLLOCATED_INTERFERENCE_REQ = 11,
	WNM_COLLOCATED_INTERFERENCE_REPORT = 12,
	WNM_TFS_REQ = 13,
	WNM_TFS_RESP = 14,
	WNM_TFS_NOTIFY = 15,
	WNM_SLEEP_MODE_REQ = 16,
	WNM_SLEEP_MODE_RESP = 17,
	WNM_TIM_BROADCAST_REQ = 18,
	WNM_TIM_BROADCAST_RESP = 19,
	WNM_QOS_TRAFFIC_CAPAB_UPDATE = 20,
	WNM_CHANNEL_USAGE_REQ = 21,
	WNM_CHANNEL_USAGE_RESP = 22,
	WNM_DMS_REQ = 23,
	WNM_DMS_RESP = 24,
	WNM_TIMING_MEASUREMENT_REQ = 25,
	WNM_NOTIFICATION_REQ = 26,
	WNM_NOTIFICATION_RESP = 27
};

/* IEEE 802.11v - BSS Transition Management Request - Request Mode */
#define WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED BIT(0)
#define WNM_BSS_TM_REQ_ABRIDGED BIT(1)
#define WNM_BSS_TM_REQ_DISASSOC_IMMINENT BIT(2)
#define WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED BIT(3)
#define WNM_BSS_TM_REQ_ESS_DISASSOC_IMMINENT BIT(4)

/* IEEE Std 802.11-2012 - Table 8-253 */
enum bss_trans_mgmt_status_code {
	WNM_BSS_TM_ACCEPT = 0,
	WNM_BSS_TM_REJECT_UNSPECIFIED = 1,
	WNM_BSS_TM_REJECT_INSUFFICIENT_BEACON = 2,
	WNM_BSS_TM_REJECT_INSUFFICIENT_CAPABITY = 3,
	WNM_BSS_TM_REJECT_UNDESIRED = 4,
	WNM_BSS_TM_REJECT_DELAY_REQUEST = 5,
	WNM_BSS_TM_REJECT_STA_CANDIDATE_LIST_PROVIDED = 6,
	WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES = 7,
	WNM_BSS_TM_REJECT_LEAVING_ESS = 8
};

#define WNM_NEIGHBOR_TSF                         1
#define WNM_NEIGHBOR_CONDENSED_COUNTRY_STRING    2
#define WNM_NEIGHBOR_BSS_TRANSITION_CANDIDATE    3
#define WNM_NEIGHBOR_BSS_TERMINATION_DURATION    4
#define WNM_NEIGHBOR_BEARING                     5
#define WNM_NEIGHBOR_MEASUREMENT_PILOT          66
#define WNM_NEIGHBOR_RRM_ENABLED_CAPABILITIES   70
#define WNM_NEIGHBOR_MULTIPLE_BSSID             71

/* QoS action */
enum qos_action {
	QOS_ADDTS_REQ = 0,
	QOS_ADDTS_RESP = 1,
	QOS_DELTS = 2,
	QOS_SCHEDULE = 3,
	QOS_QOS_MAP_CONFIG = 4,
};

/* IEEE Std 802.11-2012, 8.4.2.62 20/40 BSS Coexistence element */
#define WLAN_20_40_BSS_COEX_INFO_REQ            BIT(0)
#define WLAN_20_40_BSS_COEX_40MHZ_INTOL         BIT(1)
#define WLAN_20_40_BSS_COEX_20MHZ_WIDTH_REQ     BIT(2)
#define WLAN_20_40_BSS_COEX_OBSS_EXEMPT_REQ     BIT(3)
#define WLAN_20_40_BSS_COEX_OBSS_EXEMPT_GRNT    BIT(4)

struct ieee80211_2040_bss_coex_ie {
	u8 element_id;
	u8 length;
	u8 coex_param;
} STRUCT_PACKED;

struct ieee80211_2040_intol_chan_report {
	u8 element_id;
	u8 length;
	u8 op_class;
	u8 variable[0];	/* Channel List */
} STRUCT_PACKED;

/* IEEE 802.11v - WNM-Sleep Mode element */
struct wnm_sleep_element {
	u8 eid;     /* WLAN_EID_WNMSLEEP */
	u8 len;
	u8 action_type; /* WNM_SLEEP_ENTER/WNM_SLEEP_MODE_EXIT */
	u8 status;
	le16 intval;
} STRUCT_PACKED;

#define WNM_SLEEP_MODE_ENTER 0
#define WNM_SLEEP_MODE_EXIT 1

enum wnm_sleep_mode_response_status {
	WNM_STATUS_SLEEP_ACCEPT = 0,
	WNM_STATUS_SLEEP_EXIT_ACCEPT_GTK_UPDATE = 1,
	WNM_STATUS_DENIED_ACTION = 2,
	WNM_STATUS_DENIED_TMP = 3,
	WNM_STATUS_DENIED_KEY = 4,
	WNM_STATUS_DENIED_OTHER_WNM_SERVICE = 5
};

/* WNM-Sleep Mode subelement IDs */
enum wnm_sleep_mode_subelement_id {
	WNM_SLEEP_SUBELEM_GTK = 0,
	WNM_SLEEP_SUBELEM_IGTK = 1
};

/* Channel Switch modes (802.11h) */
#define CHAN_SWITCH_MODE_ALLOW_TX	0
#define CHAN_SWITCH_MODE_BLOCK_TX	1

struct tpc_report {
	u8 eid;
	u8 len;
	u8 tx_power;
	u8 link_margin;
} STRUCT_PACKED;

/* IEEE Std 802.11-2012, 8.5.7.4 - Link Measurement Request frame format */
struct rrm_link_measurement_request {
	u8 dialog_token;
	s8 tx_power;
	s8 max_tp;
	u8 variable[0];
} STRUCT_PACKED;

/* IEEE Std 802.11-2012, 8.5.7.5 - Link Measurement Report frame format */
struct rrm_link_measurement_report {
	u8 dialog_token;
	struct tpc_report tpc;
	u8 rx_ant_id;
	u8 tx_ant_id;
	u8 rcpi;
	u8 rsni;
	u8 variable[0];
} STRUCT_PACKED;

#endif /* IEEE802_11_DEFS_H */
