/* $NetBSD: tlv.h,v 1.3 2013/01/26 17:29:55 kefren Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#ifndef _TLV_H_
#define _TLV_H_

/* TLV messages */
#define	TLV_FEC			0x0100
#define	TLV_ADDRESS_LIST	0x0101
#define	TLV_HOP_COUNT		0x0103
#define	TLV_PATH_VECTOR		0x0104
#define	TLV_GENERIC_LABEL	0x0200
#define	TLV_ATM_LABEL		0x0201
#define	TLV_FR_LABEL		0x0202
#define	TLV_STATUS		0x0300
#define	TLV_EXTENDED_STATUS	0x0301
#define	TLV_RETURNED_PDU	0x0302
#define	TLV_RETURNED_MESSAGE	0x0303
#define	TLV_COMMON_HELLO	0x0400
#define	TLV_IPV4_TRANSPORT	0x0401
#define	TLV_CONFIGURATION_SEQ	0x0402
#define	TLV_IPV6_TRANSPORT	0x0403
#define	TLV_COMMON_SESSION	0x0500
#define	TLV_ATM_SESSION		0x0501
#define	TLV_FR_SESSION		0x0502
#define	TLV_LABEL_REQUEST	0x0600

/* Some common lengths in order to avoid writing them every time */
#define	TLV_TYPE_LENGTH (sizeof(uint16_t) + sizeof(uint16_t))
#define	MSGID_SIZE (sizeof(uint32_t))

/* General TLV structure */
struct tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
	void           *value;
	struct ldp_pdu *pdu;
}               __packed;

/* Common Hello TLV structure */
struct common_hello_tlv {
	uint16_t       type;
	uint16_t       length;
	uint16_t       holdtime;
	union {
		/* XXX: Endianness ?! */
		uint8_t        tr:2;
		uint16_t       res;
	};
}               __packed;

/* Hello TLV structure */
struct hello_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
	struct common_hello_tlv ch;
	/* XXX: optional parameters */
}               __packed;

/* Transport address TLV */
struct transport_address_tlv {
	uint16_t       type;
	uint16_t       length;
	union {
		struct in_addr  ip4addr;
		struct in6_addr ip6addr;
	} address;
}               __packed;


#define CS_LEN (sizeof(struct init_tlv) - TLV_TYPE_LENGTH - MSGID_SIZE - \
		sizeof(uint32_t))

/* Initialization TLV structure */
struct init_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
	/*
	 * Common Session Parameters
	 */
	uint16_t       cs_type;
	uint16_t       cs_len;
	uint16_t       cs_version;
	uint16_t       cs_keepalive;
	uint16_t       cs_adpvlim;	/* XXX */
	uint16_t       cs_maxpdulen;
	struct in_addr  cs_peeraddress;
	uint16_t       cs_peeraddrspace;
}               __packed;

/* Keepalive TLV */
struct ka_tlv {			/* Keepalive message */
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
}               __packed;

/* Notification TLV */
struct notification_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
	uint16_t       status;
	uint16_t       st_len;
	uint32_t       st_code;
	uint32_t       msg_id;
	uint32_t       msg_type;
}               __packed;

/* Address LIST TLV for SEND */
struct address_list_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
	uint16_t       a_type;
	uint16_t       a_length;
	uint16_t       a_af;
	struct in_addr  a_address;
}               __packed;

/* Real AL TLV used for RCV for now */
struct al_tlv {
	uint16_t       type;
	uint16_t       length;
	uint16_t       af;
	struct in_addr  address;
}               __packed;

struct address_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
}               __packed;

/* Label map TLV */
struct label_map_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       messageid;
}               __packed;

/* FEC TLV */
struct fec_tlv {
	uint16_t       type;
	uint16_t       length;
}               __packed;

struct prefix_tlv {
	uint8_t        type;
	uint16_t       af;
	uint8_t        prelen;
	struct in_addr  prefix;
}               __packed;

struct host_tlv {
	uint8_t        type;
	uint16_t       af;
	uint8_t        length;
	struct in_addr  address;
}               __packed;

struct label_tlv {
	uint16_t       type;
	uint16_t       length;
	uint32_t       label;
}               __packed;

/* Label Request Message ID TLV */
struct label_request_tlv {
	uint16_t	type;
	uint16_t	length;	/* 4 */
	uint32_t	messageid;
}		__packed;

struct hello_tlv *	get_hello_tlv(unsigned char *, uint);
void			debug_tlv(struct tlv *);

#endif	/* !_TLV_H_ */
