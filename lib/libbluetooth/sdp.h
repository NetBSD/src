/*	$NetBSD: sdp.h,v 1.2 2009/07/25 17:04:51 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2001-2003, 2008 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libsdp/sdp.h,v 1.5 2005/05/27 19:11:33 emax Exp $
 */

#ifndef _SDP_H_
#define _SDP_H_

#include <stdbool.h>
#include <uuid.h>

#include <bluetooth.h>

/*
 * SDP Data Element types
 */

#define SDP_DATA_TYPE(b)	((b) & 0xf8)
#define SDP_DATA_SIZE(b)	((b) & 0x07)

/* data size descriptors */
#define SDP_DATA_8		0x00
#define SDP_DATA_16		0x01
#define SDP_DATA_32		0x02
#define SDP_DATA_64		0x03
#define SDP_DATA_128		0x04
#define SDP_DATA_EXT8		0x05
#define SDP_DATA_EXT16		0x06
#define SDP_DATA_EXT32		0x07

/* data type descriptors */
#define SDP_DATA_NIL		0x00
#define SDP_DATA_UINT		0x08
#define SDP_DATA_INT		0x10
#define SDP_DATA_UUID		0x18
#define SDP_DATA_STR		0x20
#define SDP_DATA_BOOL		0x28
#define SDP_DATA_SEQ		0x30
#define SDP_DATA_ALT		0x38
#define SDP_DATA_URL		0x40

/* Unsigned integer */
#define SDP_DATA_UINT8		(SDP_DATA_UINT | SDP_DATA_8)
#define SDP_DATA_UINT16		(SDP_DATA_UINT | SDP_DATA_16)
#define SDP_DATA_UINT32		(SDP_DATA_UINT | SDP_DATA_32)
#define SDP_DATA_UINT64		(SDP_DATA_UINT | SDP_DATA_64)
#define SDP_DATA_UINT128	(SDP_DATA_UINT | SDP_DATA_128)

/* Signed two's-complement integer */
#define SDP_DATA_INT8		(SDP_DATA_INT | SDP_DATA_8)
#define SDP_DATA_INT16		(SDP_DATA_INT | SDP_DATA_16)
#define SDP_DATA_INT32		(SDP_DATA_INT | SDP_DATA_32)
#define SDP_DATA_INT64		(SDP_DATA_INT | SDP_DATA_64)
#define SDP_DATA_INT128		(SDP_DATA_INT | SDP_DATA_128)

/* UUID, a universally unique identifier */
#define SDP_DATA_UUID16		(SDP_DATA_UUID | SDP_DATA_16)
#define SDP_DATA_UUID32		(SDP_DATA_UUID | SDP_DATA_32)
#define SDP_DATA_UUID128	(SDP_DATA_UUID | SDP_DATA_128)

/* Text string */
#define SDP_DATA_STR8		(SDP_DATA_STR | SDP_DATA_EXT8)
#define SDP_DATA_STR16		(SDP_DATA_STR | SDP_DATA_EXT16)
#define SDP_DATA_STR32		(SDP_DATA_STR | SDP_DATA_EXT32)

/* Data element sequence */
#define SDP_DATA_SEQ8		(SDP_DATA_SEQ | SDP_DATA_EXT8)
#define SDP_DATA_SEQ16		(SDP_DATA_SEQ | SDP_DATA_EXT16)
#define SDP_DATA_SEQ32		(SDP_DATA_SEQ | SDP_DATA_EXT32)

/* Data element alternative */
#define SDP_DATA_ALT8		(SDP_DATA_ALT | SDP_DATA_EXT8)
#define SDP_DATA_ALT16		(SDP_DATA_ALT | SDP_DATA_EXT16)
#define SDP_DATA_ALT32		(SDP_DATA_ALT | SDP_DATA_EXT32)

/* URL, a uniform resource locator */
#define SDP_DATA_URL8		(SDP_DATA_URL | SDP_DATA_EXT8)
#define SDP_DATA_URL16		(SDP_DATA_URL | SDP_DATA_EXT16)
#define SDP_DATA_URL32		(SDP_DATA_URL | SDP_DATA_EXT32)

/*
 * Protocol UUIDs (short alias version)
 *
 * BLUETOOTH BASE UUID 00000000-0000-1000-8000-00805F9B34FB
 */
#define SDP_UUID_PROTOCOL_SDP				0x0001
#define SDP_UUID_PROTOCOL_UDP				0x0002
#define SDP_UUID_PROTOCOL_RFCOMM			0x0003
#define SDP_UUID_PROTOCOL_TCP				0x0004
#define SDP_UUID_PROTOCOL_TCS_BIN			0x0005
#define SDP_UUID_PROTOCOL_TCS_AT			0x0006
#define SDP_UUID_PROTOCOL_OBEX				0x0008
#define SDP_UUID_PROTOCOL_IP				0x0009
#define SDP_UUID_PROTOCOL_FTP				0x000A
#define SDP_UUID_PROTOCOL_HTTP				0x000C
#define SDP_UUID_PROTOCOL_WSP				0x000E
#define SDP_UUID_PROTOCOL_BNEP				0x000F
#define SDP_UUID_PROTOCOL_UPNP				0x0010
#define SDP_UUID_PROTOCOL_HIDP				0x0011
#define SDP_UUID_PROTOCOL_HARDCOPY_CONTROL_CHANNEL	0x0012
#define SDP_UUID_PROTOCOL_HARDCOPY_DATA_CHANNEL		0x0014
#define SDP_UUID_PROTOCOL_HARDCOPY_NOTIFICATION		0x0016
#define SDP_UUID_PROTOCOL_AVCTP				0x0017
#define SDP_UUID_PROTOCOL_AVDTP				0x0019
#define SDP_UUID_PROTOCOL_CMPT				0x001B
#define SDP_UUID_PROTOCOL_UDI_C_PLANE			0x001D
#define SDP_UUID_PROTOCOL_MCAP_CONTROL_CHANNEL		0x001E
#define SDP_UUID_PROTOCOL_MCAP_DATA_CHANNEL		0x001F
#define SDP_UUID_PROTOCOL_L2CAP				0x0100

/*
 * Service Class IDs
 */
#define SDP_SERVICE_CLASS_SERVICE_DISCOVERY_SERVER	0x1000
#define SDP_SERVICE_CLASS_BROWSE_GROUP_DESCRIPTOR	0x1001
#define SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP		0x1002
#define SDP_SERVICE_CLASS_SERIAL_PORT			0x1101
#define SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP		0x1102
#define SDP_SERVICE_CLASS_DIALUP_NETWORKING		0x1103
#define SDP_SERVICE_CLASS_IR_MC_SYNC			0x1104
#define SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH		0x1105
#define SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER		0x1106
#define SDP_SERVICE_CLASS_IR_MC_SYNC_COMMAND		0x1107
#define SDP_SERVICE_CLASS_HEADSET			0x1108
#define SDP_SERVICE_CLASS_CORDLESS_TELEPHONY		0x1109
#define SDP_SERVICE_CLASS_AUDIO_SOURCE			0x110A
#define SDP_SERVICE_CLASS_AUDIO_SINK			0x110B
#define SDP_SERVICE_CLASS_AV_REMOTE_CONTROL_TARGET	0x110C
#define SDP_SERVICE_CLASS_ADVANCED_AUDIO_DISTRIBUTION	0x110D
#define SDP_SERVICE_CLASS_AV_REMOTE_CONTROL		0x110E
#define SDP_SERVICE_CLASS_VIDEO_CONFERENCING		0x110F
#define SDP_SERVICE_CLASS_INTERCOM			0x1110
#define SDP_SERVICE_CLASS_FAX				0x1111
#define SDP_SERVICE_CLASS_HEADSET_AUDIO_GATEWAY		0x1112
#define SDP_SERVICE_CLASS_WAP				0x1113
#define SDP_SERVICE_CLASS_WAP_CLIENT			0x1114
#define SDP_SERVICE_CLASS_PANU				0x1115
#define SDP_SERVICE_CLASS_NAP				0x1116
#define SDP_SERVICE_CLASS_GN				0x1117
#define SDP_SERVICE_CLASS_DIRECT_PRINTING		0x1118
#define SDP_SERVICE_CLASS_REFERENCE_PRINTING		0x1119
#define SDP_SERVICE_CLASS_IMAGING			0x111A
#define SDP_SERVICE_CLASS_IMAGING_RESPONDER		0x111B
#define SDP_SERVICE_CLASS_IMAGING_AUTOMATIC_ARCHIVE	0x111C
#define SDP_SERVICE_CLASS_IMAGING_REFERENCED_OBJECTS	0x111D
#define SDP_SERVICE_CLASS_HANDSFREE			0x111E
#define SDP_SERVICE_CLASS_HANDSFREE_AUDIO_GATEWAY	0x111F
#define SDP_SERVICE_CLASS_DIRECT_PRINTING_REFERENCE_OBJECTS	0x1120
#define SDP_SERVICE_CLASS_REFLECTED_UI			0x1121
#define SDP_SERVICE_CLASS_BASIC_PRINTING		0x1122
#define SDP_SERVICE_CLASS_PRINTING_STATUS		0x1123
#define SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE	0x1124
#define SDP_SERVICE_CLASS_HARDCOPY_CABLE_REPLACEMENT	0x1125
#define SDP_SERVICE_CLASS_HCR_PRINT			0x1126
#define SDP_SERVICE_CLASS_HCR_SCAN			0x1127
#define SDP_SERVICE_CLASS_COMMON_ISDN_ACCESS		0x1128
#define SDP_SERVICE_CLASS_VIDEO_CONFERENCING_GW		0x1129
#define SDP_SERVICE_CLASS_UDI_MT			0x112A
#define SDP_SERVICE_CLASS_UDI_TA			0x112B
#define SDP_SERVICE_CLASS_AUDIO_VIDEO			0x112C
#define SDP_SERVICE_CLASS_SIM_ACCESS			0x112D
#define SDP_SERVICE_CLASS_PHONEBOOK_ACCESS_PCE		0x112E
#define SDP_SERVICE_CLASS_PHONEBOOK_ACCESS_PSE		0x112F
#define SDP_SERVICE_CLASS_PHONEBOOK_ACCESS		0x1130
#define SDP_SERVICE_CLASS_HEADSET_HS			0x1131
#define SDP_SERVICE_CLASS_MESSAGE_ACCESS_SERVER		0x1132
#define SDP_SERVICE_CLASS_MESSAGE_NOTIFICATION_SERVER	0x1133
#define SDP_SERVICE_CLASS_MESSAGE_ACCESS_PROFILE	0x1134
#define SDP_SERVICE_CLASS_PNP_INFORMATION		0x1200
#define SDP_SERVICE_CLASS_GENERIC_NETWORKING		0x1201
#define SDP_SERVICE_CLASS_GENERIC_FILE_TRANSFER		0x1202
#define SDP_SERVICE_CLASS_GENERIC_AUDIO			0x1203
#define SDP_SERVICE_CLASS_GENERIC_TELEPHONY		0x1204
#define SDP_SERVICE_CLASS_UPNP				0x1205
#define SDP_SERVICE_CLASS_UPNP_IP			0x1206
#define SDP_SERVICE_CLASS_ESDP_UPNP_IP_PAN		0x1300
#define SDP_SERVICE_CLASS_ESDP_UPNP_IP_LAP		0x1301
#define SDP_SERVICE_CLASS_ESDP_UPNP_L2CAP		0x1302
#define SDP_SERVICE_CLASS_VIDEO_SOURCE			0x1303
#define SDP_SERVICE_CLASS_VIDEO_SINK			0x1304
#define SDP_SERVICE_CLASS_VIDEO_DISTRIBUTION		0x1305
#define SDP_SERVICE_CLASS_HDP				0x1400
#define SDP_SERVICE_CLASS_HDP_SOURCE			0x1401
#define SDP_SERVICE_CLASS_HDP_SINK			0x1402

/*
 * Universal Attribute definitions valid in all service classes
 */
#define SDP_ATTR_SERVICE_RECORD_HANDLE			0x0000
#define SDP_ATTR_SERVICE_CLASS_ID_LIST			0x0001
#define SDP_ATTR_SERVICE_RECORD_STATE			0x0002
#define SDP_ATTR_SERVICE_ID				0x0003
#define SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST		0x0004
#define SDP_ATTR_BROWSE_GROUP_LIST			0x0005
#define SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST	0x0006
#define SDP_ATTR_SERVICE_INFO_TIME_TO_LIVE		0x0007
#define SDP_ATTR_SERVICE_AVAILABILITY			0x0008
#define SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST	0x0009
#define SDP_ATTR_DOCUMENTATION_URL			0x000A
#define SDP_ATTR_CLIENT_EXECUTABLE_URL			0x000B
#define SDP_ATTR_ICON_URL				0x000C
#define SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS	0x000D
/* 0x000E-0x01ff are reserved */

/*
 * The offset must be added to the attribute ID base (contained in the
 * LANGUAGE_BASE_ATTRIBUTE_ID_LIST attribute value) in order to compute
 * the attribute ID for these attributes.
 * The primary language base is always 0x0100.
 */
#define SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID		0x0100
#define SDP_ATTR_SERVICE_NAME_OFFSET			0x0000
#define SDP_ATTR_SERVICE_DESCRIPTION_OFFSET		0x0001
#define SDP_ATTR_PROVIDER_NAME_OFFSET			0x0002

/* ServiceDiscoveryServer profile attribute definitions */
#define SDP_ATTR_VERSION_NUMBER_LIST			0x0200
#define SDP_ATTR_SERVICE_DATABASE_STATE			0x0201

/* BrowseGroupDescriptor profile attribute definitions */
#define SDP_ATTR_GROUP_ID				0x0200

/* Other profile attribute definitions */
#define SDP_ATTR_IP_SUBNET				0x0200
#define SDP_ATTR_SUPPORT_FEATURES_LIST			0x0200
#define SDP_ATTR_SERVICE_VERSION			0x0300
#define SDP_ATTR_DATA_EXCHANGE_SPECIFICATION		0x0301
#define SDP_ATTR_EXTERNAL_NETWORK			0x0301
#define SDP_ATTR_NETWORK				0x0301
#define SDP_ATTR_SUPPORTED_DATA_STORES_LIST		0x0301
#define SDP_ATTR_FAX_CLASS1_SUPPORT			0x0302
#define SDP_ATTR_REMOTE_AUDIO_VOLUME_CONTROL		0x0302
#define SDP_ATTR_MCAP_SUPPORTED_PROCEDURES		0x0302
#define SDP_ATTR_FAX_CLASS20_SUPPORT			0x0303
#define SDP_ATTR_SUPPORTED_FORMATS_LIST			0x0303
#define SDP_ATTR_FAX_CLASS2_SUPPORT			0x0304
#define SDP_ATTR_AUDIO_FEEDBACK_SUPPORT			0x0305
#define SDP_ATTR_NETWORK_ADDRESS			0x0306
#define SDP_ATTR_WAP_GATEWAY				0x0307
#define SDP_ATTR_HOME_PAGE_URL				0x0308
#define SDP_ATTR_WAP_STACK_TYPE				0x0309
#define SDP_ATTR_SECURITY_DESCRIPTION			0x030A
#define SDP_ATTR_NET_ACCESS_TYPE			0x030B
#define SDP_ATTR_MAX_NET_ACCESS_RATE			0x030C
#define SDP_ATTR_IPV4_SUBNET				0x030D
#define SDP_ATTR_IPV6_SUBNET				0x030E
#define SDP_ATTR_SUPPORTED_CAPABALITIES			0x0310
#define SDP_ATTR_SUPPORTED_FEATURES			0x0311
#define SDP_ATTR_SUPPORTED_FUNCTIONS			0x0312
#define SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY		0x0313
#define SDP_ATTR_SUPPORTED_REPOSITORIES			0x0314
#define SDP_ATTR_MAS_INSTANCE_ID			0x0315
#define SDP_ATTR_SUPPORTED_MESSAGE_TYPES		0x0316

/*
 * Protocol Data Unit (PDU) header format
 */

typedef struct {
	uint8_t		pid;	/* PDU ID - SDP_PDU_xxx */
	uint16_t	tid;	/* transaction ID */
	uint16_t	len;	/* parameters length (in bytes) */
} __packed sdp_pdu_t;

/* PDU IDs */
#define SDP_PDU_ERROR_RESPONSE				0x01
#define SDP_PDU_SERVICE_SEARCH_REQUEST			0x02
#define SDP_PDU_SERVICE_SEARCH_RESPONSE			0x03
#define SDP_PDU_SERVICE_ATTRIBUTE_REQUEST		0x04
#define SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE		0x05
#define SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_REQUEST	0x06
#define SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_RESPONSE	0x07

/* Error codes for SDP_PDU_ERROR_RESPONSE */
#define SDP_ERROR_CODE_INVALID_SDP_VERSION		0x0001
#define SDP_ERROR_CODE_INVALID_SERVICE_RECORD_HANDLE	0x0002
#define SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX		0x0003
#define SDP_ERROR_CODE_INVALID_PDU_SIZE			0x0004
#define SDP_ERROR_CODE_INVALID_CONTINUATION_STATE	0x0005
#define SDP_ERROR_CODE_INSUFFICIENT_RESOURCES		0x0006


/*****************************************************************************
 *	sdpd(8) interface
 */

#define SDP_LOCAL_PATH	"/var/run/sdp"
#define SDP_LOCAL_MTU	L2CAP_MTU_DEFAULT

/*
 * These are NOT defined in spec and the requests are only accepted
 * by sdpd(8) on the control socket. The response will be an Error
 * Response with an ErrorCode of 0x0000 indicating success.
 */
#define SDP_PDU_RECORD_INSERT_REQUEST			0x84
#define SDP_PDU_RECORD_UPDATE_REQUEST			0x86
#define SDP_PDU_RECORD_REMOVE_REQUEST			0x82


/******************************************************************************
 *	Service Discovery API in libbluetooth(3)
 */

typedef struct {
	uint8_t *next;
	uint8_t *end;
} sdp_data_t;

typedef struct sdp_session *sdp_session_t;

/* sdp_uuid.c */
extern const uuid_t BLUETOOTH_BASE_UUID;

#ifndef SDP_COMPAT
/* sdp_session.c */
sdp_session_t sdp_open(const bdaddr_t *, const bdaddr_t *) __RENAME(_sdp_open);
sdp_session_t sdp_open_local(const char *) __RENAME(_sdp_open_local);
void sdp_close(sdp_session_t) __RENAME(_sdp_close);

/* sdp_record.c */
bool sdp_record_insert(sdp_session_t, bdaddr_t *, uint32_t *, const sdp_data_t *);
bool sdp_record_update(sdp_session_t, uint32_t, const sdp_data_t *);
bool sdp_record_remove(sdp_session_t, uint32_t);

/* sdp_service.c */
bool sdp_service_search(sdp_session_t, const sdp_data_t *, uint32_t *, int *);
bool sdp_service_attribute(sdp_session_t, uint32_t, const sdp_data_t *, sdp_data_t *);
bool sdp_service_search_attribute(sdp_session_t, const sdp_data_t *, const sdp_data_t *, sdp_data_t *);
#endif

/* sdp_match.c */
bool sdp_match_uuid16(sdp_data_t *, uint16_t);

/* sdp_get.c */
bool sdp_get_data(sdp_data_t *, sdp_data_t *);
bool sdp_get_attr(sdp_data_t *, uint16_t *, sdp_data_t *);
bool sdp_get_uuid(sdp_data_t *, uuid_t *);
bool sdp_get_bool(sdp_data_t *, bool *);
bool sdp_get_seq(sdp_data_t *, sdp_data_t *);
bool sdp_get_alt(sdp_data_t *, sdp_data_t *);
bool sdp_get_uint(sdp_data_t *, uintmax_t *);
bool sdp_get_int(sdp_data_t *, intmax_t *);
bool sdp_get_str(sdp_data_t *, char **, size_t *);
bool sdp_get_url(sdp_data_t *, char **, size_t *);

/* sdp_put.c */
bool sdp_put_data(sdp_data_t *, sdp_data_t *);
bool sdp_put_attr(sdp_data_t *, uint16_t, sdp_data_t *);
bool sdp_put_uuid(sdp_data_t *, const uuid_t *);
bool sdp_put_uuid16(sdp_data_t *, uint16_t);
bool sdp_put_uuid32(sdp_data_t *, uint32_t);
bool sdp_put_uuid128(sdp_data_t *, const uuid_t *);
bool sdp_put_bool(sdp_data_t *, bool);
bool sdp_put_uint(sdp_data_t *, uintmax_t);
bool sdp_put_uint8(sdp_data_t *, uint8_t);
bool sdp_put_uint16(sdp_data_t *, uint16_t);
bool sdp_put_uint32(sdp_data_t *, uint32_t);
bool sdp_put_uint64(sdp_data_t *, uint64_t);
bool sdp_put_int(sdp_data_t *, intmax_t);
bool sdp_put_int8(sdp_data_t *, int8_t);
bool sdp_put_int16(sdp_data_t *, int16_t);
bool sdp_put_int32(sdp_data_t *, int32_t);
bool sdp_put_int64(sdp_data_t *, int64_t);
bool sdp_put_seq(sdp_data_t *, ssize_t);
bool sdp_put_alt(sdp_data_t *, ssize_t);
bool sdp_put_str(sdp_data_t *, const char *, ssize_t);
bool sdp_put_url(sdp_data_t *, const char *, ssize_t);

/* sdp_set.c */
bool sdp_set_bool(const sdp_data_t *, bool);
bool sdp_set_uint(const sdp_data_t *, uintmax_t);
bool sdp_set_int(const sdp_data_t *, intmax_t);
bool sdp_set_seq(const sdp_data_t *, ssize_t);
bool sdp_set_alt(const sdp_data_t *, ssize_t);

/* sdp_data.c */
ssize_t sdp_data_size(const sdp_data_t *);
int sdp_data_type(const sdp_data_t *);
bool sdp_data_valid(const sdp_data_t *);
void sdp_data_print(const sdp_data_t *, int);

/******************************************************************************
 *
 *	Source level compatibility with old API
 *
 * enable with -DSDP_COMPAT but it will be removed eventually
 *
 ******************************************************************************/

#ifdef SDP_COMPAT
#include <strings.h>

typedef sdp_pdu_t *sdp_pdu_p;

#define SDP_PDU_SERVICE_REGISTER_REQUEST		0x81
#define SDP_PDU_SERVICE_UNREGISTER_REQUEST		0x82
#define SDP_PDU_SERVICE_CHANGE_REQUEST			0x83

/*
 * SDP int128/uint128 parameter
 */

struct int128 {
	int8_t	b[16];
};
typedef struct int128	int128_t;
typedef struct int128	uint128_t;

/*
 * SDP attribute
 */

struct sdp_attr {
	uint16_t	 flags;
#define SDP_ATTR_OK		(0 << 0)
#define SDP_ATTR_INVALID	(1 << 0)
#define SDP_ATTR_TRUNCATED	(1 << 1)
	uint16_t	 attr;  /* SDP_ATTR_xxx */
	uint32_t	 vlen;	/* length of the value[] in bytes */
	uint8_t		*value;	/* base pointer */
};
typedef struct sdp_attr		sdp_attr_t;
typedef struct sdp_attr *	sdp_attr_p;

#define SDP_ATTR_RANGE(lo, hi) \
	(uint32_t)(((uint16_t)(lo) << 16) | ((uint16_t)(hi)))

/* sdp_compat.c */
void *sdp_open(bdaddr_t const *, bdaddr_t const *);
void *sdp_open_local(char const *);
int32_t sdp_close(void *);
int32_t sdp_error(void *);
int32_t sdp_search(void *, uint32_t, uint16_t const *, uint32_t, uint32_t const *, uint32_t, sdp_attr_t *);
int32_t	sdp_register_service(void *, uint16_t, bdaddr_t *, uint8_t *, uint32_t, uint32_t *);
int32_t	sdp_unregister_service(void *, uint32_t);
int32_t	sdp_change_service(void *, uint32_t, uint8_t *, uint32_t);
const char *sdp_attr2desc(uint16_t);
const char *sdp_uuid2desc(uint16_t);
void sdp_print(uint32_t, uint8_t *, uint8_t const *);

/* Inline versions of get/put byte/short/long. Pointer is advanced */
#define SDP_GET8(b, cp)		do {			\
	(b) = *(const uint8_t *)(cp);			\
	(cp) += sizeof(uint8_t);			\
} while (/* CONSTCOND */0)

#define SDP_GET16(s, cp)	do {			\
	(s) = be16dec(cp);				\
	(cp) += sizeof(uint16_t);			\
} while (/* CONSTCOND */0)

#define SDP_GET32(l, cp)	do {			\
	(l) = be32dec(cp);				\
	(cp) += sizeof(uint32_t);			\
} while (/* CONSTCOND */0)

#define SDP_GET64(l, cp)	do {			\
	(l) = be64dec(cp);				\
	(cp) += sizeof(uint64_t);			\
} while (/* CONSTCOND */0)

#if BYTE_ORDER == LITTLE_ENDIAN
#define SDP_GET128(l, cp)	do {			\
	register const uint8_t *t_cp = (const uint8_t *)(cp);	\
	(l)->b[15] = *t_cp++;				\
	(l)->b[14] = *t_cp++;				\
	(l)->b[13] = *t_cp++;				\
	(l)->b[12] = *t_cp++;				\
	(l)->b[11] = *t_cp++;				\
	(l)->b[10] = *t_cp++;				\
	(l)->b[9]  = *t_cp++;				\
	(l)->b[8]  = *t_cp++;				\
	(l)->b[7]  = *t_cp++;				\
	(l)->b[6]  = *t_cp++;				\
	(l)->b[5]  = *t_cp++;				\
	(l)->b[4]  = *t_cp++;				\
	(l)->b[3]  = *t_cp++;				\
	(l)->b[2]  = *t_cp++;				\
	(l)->b[1]  = *t_cp++;				\
	(l)->b[0]  = *t_cp++;				\
	(cp) += 16;					\
} while (/* CONSTCOND */0)

#define SDP_GET_UUID128(l, cp)	do {			\
	memcpy(&((l)->b), (cp), 16);			\
	(cp) += 16;					\
} while (/* CONSTCOND */0)
#elif BYTE_ORDER == BIG_ENDIAN
#define SDP_GET128(l, cp)	do {			\
	memcpy(&((l)->b), (cp), 16);			\
	(cp) += 16;					\
} while (/* CONSTCOND */0)

#define SDP_GET_UUID128(l, cp)	SDP_GET128(l, cp)
#else
#error	"Unsupported BYTE_ORDER"
#endif /* BYTE_ORDER */

#define SDP_PUT8(b, cp)		do {			\
	*(uint8_t *)(cp) = (b);				\
	(cp) += sizeof(uint8_t);			\
} while (/* CONSTCOND */0)

#define SDP_PUT16(s, cp)	do {			\
	be16enc((cp), (s));				\
	(cp) += sizeof(uint16_t);			\
} while (/* CONSTCOND */0)

#define SDP_PUT32(s, cp)	do {			\
	be32enc((cp), (s));				\
	(cp) += sizeof(uint32_t);			\
} while (/* CONSTCOND */0)

#define SDP_PUT64(s, cp)	do {			\
	be64enc((cp), (s));				\
	(cp) += sizeof(uint64_t);			\
} while (/* CONSTCOND */0)

#if BYTE_ORDER == LITTLE_ENDIAN
#define SDP_PUT128(l, cp)	do {			\
	register const uint8_t *t_cp = (const uint8_t *)(cp);	\
	*t_cp++ = (l)->b[15];				\
	*t_cp++ = (l)->b[14];				\
	*t_cp++ = (l)->b[13];				\
	*t_cp++ = (l)->b[12];				\
	*t_cp++ = (l)->b[11];				\
	*t_cp++ = (l)->b[10];				\
	*t_cp++ = (l)->b[9];				\
	*t_cp++ = (l)->b[8];				\
	*t_cp++ = (l)->b[7];				\
	*t_cp++ = (l)->b[6];				\
	*t_cp++ = (l)->b[5];				\
	*t_cp++ = (l)->b[4];				\
	*t_cp++ = (l)->b[3];				\
	*t_cp++ = (l)->b[2];				\
	*t_cp++ = (l)->b[1];				\
	*t_cp   = (l)->b[0];				\
	(cp) += 16;					\
} while (/* CONSTCOND */0)

#define SDP_PUT_UUID128(l, cp)	do {			\
	memcpy((cp), &((l)->b), 16);			\
	(cp) += 16;					\
} while (/* CONSTCOND */0)
#elif BYTE_ORDER == BIG_ENDIAN
#define SDP_PUT128(l, cp)	do {			\
	memcpy((cp), &((l)->b), 16);			\
	(cp) += 16;					\
} while (/* CONSTCOND */0)

#define SDP_PUT_UUID128(l, cp)	SDP_PUT128(l, cp)
#else
#error	"Unsupported BYTE_ORDER"
#endif /* BYTE_ORDER */

struct sdp_dun_profile
{
	uint8_t	server_channel;
	uint8_t	audio_feedback_support;
	uint8_t	reserved[2];
};
typedef struct sdp_dun_profile		sdp_dun_profile_t;
typedef struct sdp_dun_profile *	sdp_dun_profile_p;

struct sdp_ftrn_profile
{
	uint8_t	server_channel;
	uint8_t	reserved[3];
};
typedef struct sdp_ftrn_profile		sdp_ftrn_profile_t;
typedef struct sdp_ftrn_profile *	sdp_ftrn_profile_p;

struct sdp_hset_profile
{
	uint8_t server_channel;
	uint8_t	reserved[3];
};
typedef struct sdp_hset_profile		sdp_hset_profile_t;
typedef struct sdp_hset_profile *	sdp_hset_profile_p;

struct sdp_hf_profile
{
	uint8_t server_channel;
	uint16_t supported_features;
};
typedef struct sdp_hf_profile		sdp_hf_profile_t;
typedef struct sdp_hf_profile *		sdp_hf_profile_p;

/* Keep this in sync with sdp_opush_profile */
struct sdp_irmc_profile
{
	uint8_t	server_channel;
	uint8_t	supported_formats_size;
	uint8_t	supported_formats[30];
};
typedef struct sdp_irmc_profile		sdp_irmc_profile_t;
typedef struct sdp_irmc_profile *	sdp_irmc_profile_p;

struct sdp_irmc_command_profile
{
	uint8_t	server_channel;
	uint8_t	reserved[3];
};
typedef struct sdp_irmc_command_profile		sdp_irmc_command_profile_t;
typedef struct sdp_irmc_command_profile *	sdp_irmc_command_profile_p;

struct sdp_lan_profile
{
	uint8_t		server_channel;
	uint8_t		load_factor;
	uint8_t		reserved;
	uint8_t		ip_subnet_radius;
	uint32_t	ip_subnet;
};
typedef struct sdp_lan_profile		sdp_lan_profile_t;
typedef struct sdp_lan_profile *	sdp_lan_profile_p;

/* Keep this in sync with sdp_irmc_profile */
struct sdp_opush_profile
{
	uint8_t	server_channel;
	uint8_t	supported_formats_size;
	uint8_t	supported_formats[30];
};
typedef struct sdp_opush_profile	sdp_opush_profile_t;
typedef struct sdp_opush_profile *	sdp_opush_profile_p;

struct sdp_sp_profile
{
	uint8_t	server_channel;
	uint8_t	reserved[3];
};
typedef struct sdp_sp_profile	sdp_sp_profile_t;
typedef struct sdp_sp_profile *	sdp_sp_profile_p;

struct sdp_nap_profile
{
	uint8_t		reserved;
	uint8_t		load_factor;
	uint16_t	psm;
	uint16_t	security_description;
	uint16_t	net_access_type;
	uint32_t	max_net_access_rate;
};
typedef struct sdp_nap_profile		sdp_nap_profile_t;
typedef struct sdp_nap_profile *	sdp_nap_profile_p;

struct sdp_gn_profile
{
	uint8_t		reserved;
	uint8_t		load_factor;
	uint16_t	psm;
	uint16_t	security_description;
	uint16_t	reserved2;
	uint32_t	reserved3;
};
typedef struct sdp_gn_profile		sdp_gn_profile_t;
typedef struct sdp_gn_profile *		sdp_gn_profile_p;

struct sdp_panu_profile
{
	uint8_t		reserved;
	uint8_t		load_factor;
	uint16_t	psm;
	uint16_t	security_description;
	uint16_t	reserved2;
	uint32_t	reserved3;
};
typedef struct sdp_panu_profile		sdp_panu_profile_t;
typedef struct sdp_panu_profile *	sdp_panu_profile_p;

#endif /* SDP_COMPAT */

#endif /* _SDP_H_ */
