/* $NetBSD: hci_le.h,v 1.1 2024/03/13 07:22:16 nat Exp $ */

/*-
 * Copyright (c) 2020 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define HCI_ADVERT_DATA_SIZE		31  /* advertising data size */
#define HCI_SCAN_DATA_SIZE		31  /* scan resp. data size */
 
/* LE Event masks */
#define HCI_LE_EVMSK_ALL			0x000000000000001f
#define HCI_LE_EVMSK_NONE			0x0000000000000000
#define HCI_LE_EVMSK_CON_COMPL			0x0000000000000001
#define HCI_LE_EVMSK_ADV_REPORT			0x0000000000000002
#define HCI_LE_EVMSK_CON_UPDATE_COMPL		0x0000000000000004
#define HCI_LE_EVMSK_READ_REMOTE_FEATURES_COMPL	0x0000000000000008
#define HCI_LE_EVMSK_LONG_TERM_KEY_REQ		0x0000000000000010
/* 0x0000000000000020 - 0x8000000000000000 - reserved for future use */

/**************************************************************************
 **************************************************************************
 ** OGF 0x08	Bluetooth Low Energy (LE) Link commands
 **************************************************************************
 **************************************************************************/

#define HCI_OGF_LE				0x08

#define HCI_OCF_LE_SET_EVENT_MASK			0x0001
#define HCI_CMD_LE_SET_EVENT_MASK			0x2001
typedef struct {
	uint8_t		event_mask[HCI_EVENT_MASK_SIZE]; /* event_mask */
} __packed hci_le_set_event_mask_cp;

typedef hci_status_rp	hci_le_set_event_mask_rp;

#define HCI_OCF_LE_READ_BUFFER_SIZE			0x0002
#define HCI_CMD_LE_READ_BUFFER_SIZE			0x2002
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	le_data_pktlen; /* buffer len*/
	uint8_t		le_num_pkts; 	/* no. acl data packets */
} __packed hci_le_read_buffer_size_rp;

#define HCI_OCF_LE_READ_LOCAL_FEATURES			0x0003
#define HCI_CMD_LE_READ_LOCAL_FEATURES			0x2003
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint8_t		features[HCI_FEATURES_SIZE];	/* le features */
} __packed hci_le_read_local_features_rp;

#define HCI_OCF_LE_SET_RND_ADDR				0x0005
#define HCI_CMD_LE_SET_RND_ADDR				0x2005
typedef struct {
	bdaddr_t	bdaddr; 	/* random local address */
} __packed hci_le_set_rnd_addr_cp;

typedef hci_status_rp	hci_le_set_rnd_addr_rp;
/* XXX NS Finish defines. */
#define HCI_OCF_LE_SET_ADVERT_PARAM			0x0006
#define HCI_CMD_LE_SET_ADVERT_PARAM			0x2006
typedef struct {
	uint16_t	min_interval; 	/* min interval * 0.625ms */
	uint16_t	max_interval; 	/* max_interval * 0.625ms */
	uint8_t		advert_type;
	uint8_t		own_address_type;
	uint8_t		direct_address_type;
	bdaddr_t	direct_address; /* remote address */
	uint8_t		advert_channel_map;
	uint8_t		advert_filter_policy;
} __packed hci_le_set_advert_param_cp;

typedef hci_status_rp	hci_le_set_advert_param_rp;

#define HCF_OCF_LE_READ_ADVERT_CHAN_TX_PWR		0x0007
#define HCF_CMD_LE_READ_ADVERT_CHAN_TX_PWR		0x2007
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	int8_t		tx_power_level; /* -20 - 10 dBm */
} __packed hci_le_read_advert_chan_tx_pwr_rp;

#define HCF_OCF_LE_SET_ADVERT_DATA			0x0008
#define HCF_CMD_LE_SET_ADVERT_DATA			0x2008
typedef struct {
	uint8_t		advert_data_len; 	/* 0x00 - 0x1f */
	uint8_t		advert_data[HCI_ADVERT_DATA_SIZE]; /* def all 0's */
} __packed hci_le_set_advert_data_cp;

typedef hci_status_rp	hci_le_set_advert_data_rp;

#define HCF_OCF_LE_SET_SCAN_RESP_DATA			0x0009
#define HCF_CMD_LE_SET_SCAN_RESP_DATA			0x2009
typedef struct {
	uint8_t		scan_resp_data_len; 	/* 0x00 - 0x1f */
	uint8_t		scan_resp_data[HCI_SCAN_DATA_SIZE]; /* def all 0's */
} __packed hci_le_set_scan_resp_data_cp;

typedef hci_status_rp	hci_le_set_scan_resp_data_rp;

#define HCF_OCF_LE_SET_ADVERT_ENABLE			0x000a
#define HCF_CMD_LE_SET_ADVERT_ENABLE			0x200A
typedef struct {
	uint8_t		advert_enable; 	/* 0x00 - disable 0x1 - enable */
					/* 0x2 - 0xff reserved */
} __packed hci_le_set_advert_enable_cp;

typedef hci_status_rp	hci_le_set_advert_enable_rp;

#define HCI_OCF_LE_SET_SCAN_PARAM			0x000b
#define HCI_CMD_LE_SET_SCAN_PARAM			0x200B
typedef struct {
	uint8_t		scan_type;
	uint16_t	scan_interval; 	/* min interval * 0.625ms */
	uint16_t	scan_window; 	/* max_interval * 0.625ms */
	uint8_t		own_address_type;
	uint8_t		scan_filter_policy;
} __packed hci_le_set_scan_param_cp;

typedef hci_status_rp	hci_le_set_scan_param_rp;

#define HCF_OCF_LE_SET_SCAN_ENABLE			0x000c
#define HCF_CMD_LE_SET_SCAN_ENABLE			0x200C
typedef struct {
	uint8_t		scan_enable; 	/* 0x00 - disable 0x1 - enable */
					/* 0x2 - 0xff reserved */
	uint8_t		filter_dup;	/* 0x00 - no filtering 0x1 - filter */
					/* 0x2 - 0xff reserved */
} __packed hci_le_set_scan_enable_cp;

typedef hci_status_rp	hci_le_set_scan_enable_rp;

#define HCI_OCF_CREATE_CON_LE				0x000d
#define HCI_CMD_CREATE_CON_LE				0x200D
typedef struct {
	uint16_t	scan_interval; 		/* min interval * 0.625ms */
	uint16_t	scan_window; 		/* max_interval * 0.625ms */
	uint8_t		initiator_filter_policy;
	uint8_t		peer_address_type;
	bdaddr_t	peer_address; 		/* remote address */
	uint8_t		own_address_type;
	uint16_t	con_interval_min;	/* min interval * 1.25ms */
	uint16_t	con_interval_max;	/* max_interval * 1.25ms */
	uint16_t	con_latency;		/* 0x0 - 0x1f4 */
	uint16_t	supervision_timo;	/* timeout * 10ms */
	uint16_t	min_ce_length;		/* min length * 0.625ms */
	uint16_t	max_ce_length;		/* max length * 0.625ms */
} __packed hci_create_con_le_cp;
/* No return parameter(s) */

#define HCI_OCF_CREATE_CON_LE_CANCEL			0x000e
#define HCI_CMD_CREATE_CON_LE_CANCEL			0x200E
/* No command parameter(s) */

typedef hci_status_rp	hci_create_con_le_cancel_rp;

#define HCI_OCF_LE_READ_WHITE_LIST_SIZE			0x000f
#define HCI_CMD_LE_READ_WHITE_LIST_SIZE			0x200F
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 		/* status 0x00 = success */
	uint8_t		white_list_size;	/* 0x1 - 0xff */
						/* 0x0 reserved */
} __packed hci_le_read_white_list_size_rp;

#define HCI_OCF_LE_CLEAR_WHITE_LIST			0x0010
#define HCI_CMD_LE_CLEAR_WHITE_LIST			0x2010
/* No command parameter(s) */

typedef hci_status_rp	hci_le_clear_white_list_rp;

#define HCI_OCF_LE_ADD_DEV_TO_WHITE_LIST		0x0011
#define HCI_CMD_LE_ADD_DEV_TO_WHITE_LIST		0x2011
typedef struct {
	uint8_t		address_type;
	bdaddr_t	address; 		/* remote address */
} __packed hci_le_add_dev_to_white_list_cp;

typedef hci_status_rp	hci_le_add_dev_to_white_list_rp;

#define HCI_OCF_LE_REMOVE_DEV_FROM_WHITE_LIST		0x0012
#define HCI_CMD_LE_REMOVE_DEV_FROM_WHITE_LIST		0x2012
typedef struct {
	uint8_t		address_type;
	bdaddr_t	address; 		/* remote address */
} __packed hci_le_remove_dev_from_white_list_cp;

typedef hci_status_rp	hci_le_remove_dev_from_white_list_rp;

#define HCI_OCF_UPDATE_CON_LE				0x0013
#define HCI_CMD_UPDATE_CON_LE				0x2013
typedef struct {
	uint16_t	con_handle;		/* handle 12 bits */
	uint16_t	con_interval_min;	/* min interval * 1.25ms */
	uint16_t	con_interval_max;	/* max_interval * 1.25ms */
	uint16_t	con_latency;		/* 0x0 - 0x1f4 */
	uint16_t	supervision_timo;	/* timeout * 10ms */
	uint16_t	min_ce_length;		/* min length * 0.625ms */
	uint16_t	max_ce_length;		/* max length * 0.625ms */
} __packed hci_update_con_le_cp;
/* No return parameter(s) */

#define HCI_OCF_LE_SET_HOST_CHAN_CLASSIFICATION		0x0014
#define HCI_CMD_LE_SET_HOST_CHAN_CLASSIFICATION		0x2014
typedef struct {
	uint8_t		map[5];
} __packed hci_le_set_host_chan_classification_cp;

typedef hci_status_rp	hci_le_set_host_chan_classification_rp;

#define HCI_OCF_LE_READ_CHANNEL_MAP			0x0015
#define HCI_CMD_LE_READ_CHANNEL_MAP			0x2015
typedef struct {
	uint16_t	con_handle; 	/* connection handle */
} __packed hci_le_read_channel_map_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	con_handle; 	/* connection handle */
	uint8_t		map[5];    	/* LE channel map */
} __packed hci_le_read_channel_map_rp;

#define HCI_OCF_LE_READ_REMOTE_FEATURES			0x0016
#define HCI_CMD_LE_READ_REMOTE_FEATURES			0x2016
typedef struct {
	uint16_t	con_handle;	/* connection handle */
} __packed hci_le_read_remote_features_cp;
/* No return parameter(s) */

#define HCI_OCF_LE_ENCRYPT				0x0017
#define HCI_CMD_LE_ENCRYPT				0x2017
typedef struct {
	uint8_t		key[16];
	uint8_t		plaintext_data[16];
} __packed hci_le_encrypt_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	enc_data[16];
} __packed hci_le_encrypt_rp;

#define HCI_OCF_LE_RAND					0x0018
#define HCI_CMD_LE_RAND					0x2018
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint8_t		rand_num[8];
} __packed hci_le_rand_rp;

#define HCI_OCF_LE_START_ENCRYPT			0x0019
#define HCI_CMD_LE_START_ENCRYPT			0x2019
typedef struct {
	uint16_t	con_handle; 	/* connection handle */
	uint8_t		rand_num[8]; 	
	uint16_t	enc_diversifier; 
	uint8_t		key[HCI_KEY_SIZE]; /* key */
} __packed hci_le_start_encrypt_cp;
/* No return parameter(s) */

#define HCI_OCF_LE_LONG_TERM_KEY_REQ_REP		0x001a
#define HCI_CMD_LE_LONG_TERM_KEY_REQ_REP		0x201A
typedef struct {
	uint16_t	con_handle; 	/* connection handle */
	uint8_t		key[HCI_KEY_SIZE]; /* key */
} __packed hci_le_long_term_key_req_rep_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	con_handle; 	/* connection handle */
} __packed hci_le_long_term_key_req_rep_rp;

#define HCI_OCF_LE_LONG_TERM_KEY_REQ_REP_NEG		0x001b
#define HCI_CMD_LE_LONG_TERM_KEY_REQ_REP_NEG		0x201B
typedef struct {
	uint16_t	con_handle; 	/* connection handle */
} __packed hci_le_long_term_key_req_rep_neg_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	con_handle; 	/* connection handle */
} __packed hci_le_long_term_key_req_rep_neg_rp;

/* XXX NS Read supported states */

#define HCI_OCF_LE_RECEIVER_TEST			0x001d
#define HCI_CMD_LE_RECEIVER_TEST			0x201D
typedef struct {
	uint8_t		rx_freq; 	/* 0x00 - 0x27 (2402 - 2480MHz) */
} __packed hci_le_receiver_test_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
} __packed hci_le_receiver_test_rp;

#define HCI_OCF_LE_TRANSMITTER_TEST			0x001e
#define HCI_CMD_LE_TRANSMITTER_TEST			0x201E
typedef struct {
	uint8_t		tx_freq; 	/* 0x00 - 0x27 (2402 - 2480MHz) */
	uint8_t		test_len;	/* 0x00 - 0x25 bytes */
					/* 0x26 - 0xff reserved */
	uint8_t		payload;	/* 0x00 - 0x02 mandatory */
					/* 0x03 - 0x07 opt. test patterns */
					/* 0x08 - 0xff reserved */
} __packed hci_le_transmitter_test_cp;

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
} __packed hci_le_transmitter_test_rp;

#define HCI_OCF_LE_TEST_END				0x001f
#define HCI_CMD_LE_TEST_END				0x201F
/* No command parameter(s) */

typedef struct {
	uint8_t		status; 	/* status 0x00 = success */
	uint16_t	num_pkts; 	/* num pkts received */
					/* 0x0000 for tx test */
} __packed hci_le_test_end_rp;

/**************************************************************************
 **************************************************************************
 **                         Events and event parameters
 **************************************************************************
 **************************************************************************/
 
#define HCI_LE_META_EVENT			0x3e
#define HCI_SUBEVT_CON_COMP			0x01
typedef struct {
	uint8_t		subevt_code;
	uint8_t		status; 		/* status 0x00 = success */
	uint16_t	con_handle;		/* handle 12 bits */
	uint8_t		role;
	uint8_t		peer_address_type;
	bdaddr_t	peer_address; 		/* remote address */
	uint8_t		own_address_type;
	uint16_t	con_interval;		/* min interval * 1.25ms */
	uint16_t	con_latency;		/* 0x0 - 0x1f4 */
	uint8_t		master_clk_accuracy;
} __packed hci_le_con_comp_ep;

