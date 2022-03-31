/*	$NetBSD: if_lagg_lacp.h,v 1.4 2022/03/31 02:04:50 yamaguchi Exp $	*/

/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
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

#ifndef _NET_LAGG_IF_LAGG_LACP_H_
#define _NET_LAGG_IF_LAGG_LACP_H_

/* timeout values (in sec) */
#define LACP_FAST_PERIODIC_TIME		1
#define LACP_SLOW_PERIODIC_TIME		30
#define LACP_SHORT_TIMEOUT_TIME		(3 * LACP_FAST_PERIODIC_TIME)
#define LACP_LONG_TIMEOUT_TIME		(3 * LACP_SLOW_PERIODIC_TIME)
#define LACP_CHURN_DETECTION_TIME	60
#define LACP_AGGREGATE_WAIT_TIME	2
#define LACP_TRANSIT_DELAY		3

#define LACP_MAX_PORTS		16
#define LACP_SYSTEM_PRIO	0x8000U
#define LACP_PORT_PRIO		LAGG_PORT_PRIO
#define LACP_SENDDU_PPS		3
#define LACP_RCVDU_LIMIT	(LACP_SENDDU_PPS * LACP_MAX_PORTS)

#define LACP_PARTNER_ADMIN_OPTIMISTIC	(LACP_STATE_SYNC | \
					LACP_STATE_AGGREGATION | \
					LACP_STATE_COLLECTING | \
					LACP_STATE_DISTRIBUTING)
#define LACP_PARTNER_ADMIN_STRICT	0

#define TLV_TYPE_TERMINATE	0

#define LACP_TYPE_TERMINATE	TLV_TYPE_TERMINATE
#define LACP_TYPE_ACTORINFO	1
#define LACP_TYPE_PARTNERINFO	2
#define LACP_TYPE_COLLECTORINFO	3

#define MARKER_TYPE_TERMINATE	TLV_TYPE_TERMINATE
#define MARKER_TYPE_INFO	1
#define MARKER_TYPE_RESPONSE	2

struct tlvhdr {
	uint8_t		 tlv_type;
	uint8_t		 tlv_length;
} __packed;

struct tlv {
	uint8_t		 tlv_t;
	uint8_t		 tlv_l;
	void		*tlv_v;
};

static inline void
tlv_set(struct tlvhdr *th, uint8_t t, uint8_t l)
{

	th->tlv_type = t;
	th->tlv_length = sizeof(*th) + l;
}

struct lacpdu_peerinfo {
	uint16_t	 lpi_system_prio;
	uint8_t		 lpi_system_mac[LACP_MAC_LEN];
	uint16_t	 lpi_key;
	uint16_t	 lpi_port_prio;
	uint16_t	 lpi_port_no;
	uint8_t		 lpi_state;
	uint8_t		 lpi_resv[3];
} __packed;

struct lacpdu_collectorinfo {
	uint16_t	 lci_maxdelay;
	uint8_t		 lci_resv[12];
} __packed;

struct lacpdu {
	struct ether_header	 ldu_eh;
	struct slowprothdr	 ldu_sph;

	struct tlvhdr		 ldu_tlv_actor;
	struct lacpdu_peerinfo	 ldu_actor;
	struct tlvhdr		 ldu_tlv_partner;
	struct lacpdu_peerinfo	 ldu_partner;

	struct tlvhdr		 ldu_tlv_collector;
	struct lacpdu_collectorinfo
				 ldu_collector;

	struct tlvhdr		 ldu_tlv_term;
	uint8_t			 ldu_resv[50];
} __packed;

struct markerdu_info {
	uint16_t	 mi_rq_port;
	uint8_t		 mi_rq_system[LACP_MAC_LEN];
	uint32_t	 mi_rq_xid;
	uint8_t		 mi_pad[2];
} __packed;

struct markerdu {
	struct ether_header	 mdu_eh;
	struct slowprothdr	 mdu_sph;

	struct tlvhdr		 mdu_tlv_info;
	struct markerdu_info	 mdu_info;

	struct tlvhdr		 mdu_tlv_term;
	uint8_t			 mdu_resv[90];
} __packed;

/*
 * lacp media:
 *   1byte
 * +-------+-------+-------+-------+
 * | media |                 speed |
 * +-------+-------+-------+-------+
 */

enum lacp_linkspeed {
	LACP_LINKSPEED_UNKNOWN = 0,
	LACP_LINKSPEED_10,
	LACP_LINKSPEED_100,
	LACP_LINKSPEED_1000,
	LACP_LINKSPEED_2500,
	LACP_LINKSPEED_5000,
	LACP_LINKSPEED_10G,
	LACP_LINKSPEED_25G,
	LACP_LINKSPEED_40G,
	LACP_LINKSPEED_50G,
	LACP_LINKSPEED_56G,
	LACP_LINKSPEED_100G,
	LACP_LINKSPEED_200G,
};

#define LACP_MEDIA_OFFSET	24
#define LACP_MEDIA_MASK		0xff000000U
#define LACP_MEDIA_ETHER	(__BIT(0) << LACP_MEDIA_OFFSET)
#define LACP_MEDIA_FDX		(__BIT(1) << LACP_MEDIA_OFFSET)
#define LACP_MEDIA_DEFAULT	(LACP_LINKSPEED_UNKNOWN |	\
				 LACP_MEDIA_ETHER | LACP_MEDIA_FDX)
#endif
