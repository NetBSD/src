/*	$KAME: sctp_header.h,v 1.14 2005/03/06 16:04:17 itojun Exp $	*/
/*	$NetBSD: sctp_header.h,v 1.2.4.2 2017/12/03 11:39:04 jdolecek Exp $ */

#ifndef __SCTP_HEADER_H__
#define __SCTP_HEADER_H__

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Cisco Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/time.h>
#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>

/*
 * Parameter structures
 */
struct sctp_ipv4addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_IPV4_PARAM_TYPE, len=8 */
	u_int32_t addr;			/* IPV4 address */
} __packed;

struct sctp_ipv6addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_IPV6_PARAM_TYPE, len=20 */
	u_int8_t  addr[16];		/* IPV6 address */
} __packed;

/* Cookie Preservative */
struct sctp_cookie_perserve_param {
	struct sctp_paramhdr ph;	/* type=SCTP_COOKIE_PRESERVE, len=8 */
	u_int32_t time;			/* time in ms to extend cookie */
} __packed;

/* Host Name Address */
struct sctp_host_name_param {
	struct sctp_paramhdr ph;	/* type=SCTP_HOSTNAME_ADDRESS */
	char name[1];			/* host name */
} __packed;

/* supported address type */
struct sctp_supported_addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_SUPPORTED_ADDRTYPE */
	u_int16_t addr_type[1];		/* array of supported address types */
} __packed;

/* ECN parameter */
struct sctp_ecn_supported_param {
	struct sctp_paramhdr ph;	/* type=SCTP_ECN_CAPABLE */
} __packed;


/* heartbeat info parameter */
struct sctp_heartbeat_info_param {
	struct sctp_paramhdr ph;
	u_int32_t time_value_1;
	u_int32_t time_value_2;
	u_int32_t random_value1;
	u_int32_t random_value2;
	u_int16_t user_req;
	u_int8_t addr_family;
	u_int8_t addr_len;
	char address[SCTP_ADDRMAX];
} __packed;


/* draft-ietf-tsvwg-prsctp */
/* PR-SCTP supported parameter */
struct sctp_prsctp_supported_param {
	struct sctp_paramhdr ph;
} __packed;


/* draft-ietf-tsvwg-addip-sctp */
struct sctp_asconf_paramhdr {		/* an ASCONF "parameter" */
	struct sctp_paramhdr ph;	/* a SCTP parameter header */
	u_int32_t correlation_id;	/* correlation id for this param */
} __packed;

struct sctp_asconf_addr_param {		/* an ASCONF address parameter */
	struct sctp_asconf_paramhdr aph;	/* asconf "parameter" */
	struct sctp_ipv6addr_param  addrp;	/* max storage size */
} __packed;

struct sctp_asconf_addrv4_param {		/* an ASCONF address (v4) parameter */
	struct sctp_asconf_paramhdr aph;	/* asconf "parameter" */
	struct sctp_ipv4addr_param  addrp;	/* max storage size */
} __packed;


/* ECN Nonce: draft-ladha-sctp-ecn-nonce */
struct sctp_ecn_nonce_supported_param {
	struct sctp_paramhdr ph;	/* type = 0x8001  len = 4 */
} __packed;

struct sctp_supported_chunk_types_param {
	struct sctp_paramhdr ph;	/* type = 0x8002  len = x */
	u_int8_t chunk_types[0];
} __packed;

/*
 * Structures for DATA chunks
 */
struct sctp_data {
	u_int32_t tsn;
	u_int16_t stream_id;
	u_int16_t stream_sequence;
	u_int32_t protocol_id;
	/* user data follows */
} __packed;

struct sctp_data_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_data dp;
} __packed;

/*
 * Structures for the control chunks
 */

/* Initiate (INIT)/Initiate Ack (INIT ACK) */
struct sctp_init {
	u_int32_t initiate_tag;		/* initiate tag */
	u_int32_t a_rwnd;		/* a_rwnd */
	u_int16_t num_outbound_streams;	/* OS */
	u_int16_t num_inbound_streams;	/* MIS */
	u_int32_t initial_tsn;		/* I-TSN */
	/* optional param's follow */
} __packed;

/* state cookie header */
struct sctp_state_cookie {		/* this is our definition... */
	u_int8_t  identification[16];	/* id of who we are */
	u_int32_t cookie_life;		/* life I will award this cookie */
	u_int32_t tie_tag_my_vtag;	/* my tag in old association */
	u_int32_t tie_tag_peer_vtag;	/* peers tag in old association */
	u_int32_t peers_vtag;		/* peers tag in INIT (for quick ref) */
	u_int32_t my_vtag;		/* my tag in INIT-ACK (for quick ref) */
	struct timeval time_entered;	/* the time I built cookie */
	u_int32_t address[4];		/* 4 ints/128 bits */
	u_int32_t addr_type;		/* address type */
	u_int32_t laddress[4];		/* my local from address */
	u_int32_t laddr_type;		/* my local from address type */
	u_int32_t scope_id;		/* v6 scope id for link-locals */
	u_int16_t peerport;		/* port address of the peer in the INIT */
	u_int16_t myport;		/* my port address used in the INIT */
	u_int8_t ipv4_addr_legal;	/* Are V4 addr legal? */
	u_int8_t ipv6_addr_legal;	/* Are V6 addr legal? */
	u_int8_t local_scope;		/* IPv6 local scope flag */
	u_int8_t site_scope;		/* IPv6 site scope flag */
	u_int8_t ipv4_scope;		/* IPv4 private addr scope */
	u_int8_t loopback_scope;	/* loopback scope information */
	u_int16_t reserved;
	/*
	 * at the end is tacked on the INIT chunk and the
	 * INIT-ACK chunk (minus the cookie).
	 */
} __packed;

struct sctp_inv_mandatory_param {
	u_int16_t cause;
	u_int16_t length;
	u_int32_t num_param;
	u_int16_t param;
	/*
	 * We include this to 0 it since only a missing cookie
	 * will cause this error.
	 */
	u_int16_t resv;
} __packed;

struct sctp_unresolv_addr {
	u_int16_t cause;
	u_int16_t length;
	u_int16_t addr_type;
	u_int16_t reserved;	/* Only one invalid addr type */
} __packed;

/* state cookie parameter */
struct sctp_state_cookie_param {
	struct sctp_paramhdr ph;
	struct sctp_state_cookie cookie;
} __packed;

struct sctp_init_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_init init;
} __packed;

struct sctp_init_msg {
	struct sctphdr sh;
	struct sctp_init_chunk msg;
} __packed;
/* ... used for both INIT and INIT ACK */
#define sctp_init_ack		sctp_init
#define sctp_init_ack_chunk	sctp_init_chunk
#define sctp_init_ack_msg	sctp_init_msg


/* Selective Ack (SACK) */
struct sctp_gap_ack_block {
	u_int16_t start;		/* Gap Ack block start */
	u_int16_t end;			/* Gap Ack block end */
} __packed;

struct sctp_sack {
	u_int32_t cum_tsn_ack;		/* cumulative TSN Ack */
	u_int32_t a_rwnd;		/* updated a_rwnd of sender */
	u_int16_t num_gap_ack_blks;	/* number of Gap Ack blocks */
	u_int16_t num_dup_tsns;		/* number of duplicate TSNs */
	/* struct sctp_gap_ack_block's follow */
	/* u_int32_t duplicate_tsn's follow */
} __packed;

struct sctp_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_sack sack;
} __packed;


/* Heartbeat Request (HEARTBEAT) */
struct sctp_heartbeat {
	struct sctp_heartbeat_info_param hb_info;
} __packed;

struct sctp_heartbeat_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_heartbeat heartbeat;
} __packed;
/* ... used for Heartbeat Ack (HEARTBEAT ACK) */
#define sctp_heartbeat_ack		sctp_heartbeat
#define sctp_heartbeat_ack_chunk	sctp_heartbeat_chunk


/* Abort Asssociation (ABORT) */
struct sctp_abort_chunk {
	struct sctp_chunkhdr ch;
	/* optional error cause may follow */
} __packed;

struct sctp_abort_msg {
	struct sctphdr sh;
	struct sctp_abort_chunk msg;
} __packed;


/* Shutdown Association (SHUTDOWN) */
struct sctp_shutdown_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t cumulative_tsn_ack;
} __packed;


/* Shutdown Acknowledgment (SHUTDOWN ACK) */
struct sctp_shutdown_ack_chunk {
	struct sctp_chunkhdr ch;
} __packed;


/* Operation Error (ERROR) */
struct sctp_error_chunk {
	struct sctp_chunkhdr ch;
	/* optional error causes follow */
} __packed;


/* Cookie Echo (COOKIE ECHO) */
struct sctp_cookie_echo_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_state_cookie cookie;
} __packed;

/* Cookie Acknowledgment (COOKIE ACK) */
struct sctp_cookie_ack_chunk {
	struct sctp_chunkhdr ch;
} __packed;

/* Explicit Congestion Notification Echo (ECNE) */
struct sctp_ecne_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t tsn;
} __packed;

/* Congestion Window Reduced (CWR) */
struct sctp_cwr_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t tsn;
} __packed;

/* Shutdown Complete (SHUTDOWN COMPLETE) */
struct sctp_shutdown_complete_chunk {
	struct sctp_chunkhdr ch;
} __packed;

/* Oper error holding a stale cookie */
struct sctp_stale_cookie_msg {
	struct sctp_paramhdr ph;	/* really an error cause */
	u_int32_t time_usec;
} __packed;

struct sctp_adaption_layer_indication {
	struct sctp_paramhdr ph;
	u_int32_t indication;
} __packed;

struct sctp_cookie_while_shutting_down {
	struct sctphdr sh;
	struct sctp_chunkhdr ch;
	struct sctp_paramhdr ph;	/* really an error cause */
} __packed;

struct sctp_shutdown_complete_msg {
	struct sctphdr sh;
	struct sctp_shutdown_complete_chunk shut_cmp;
} __packed;

/* draft-ietf-tsvwg-addip-sctp */
/* Address/Stream Configuration Change (ASCONF) */
struct sctp_asconf_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t serial_number;
	/* lookup address parameter (mandatory) */
	/* asconf parameters follow */
} __packed;

/* Address/Stream Configuration Acknowledge (ASCONF ACK) */
struct sctp_asconf_ack_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t serial_number;
	/* asconf parameters follow */
} __packed;

/* draft-ietf-tsvwg-prsctp */
/* Forward Cumulative TSN (FORWARD TSN) */
struct sctp_forward_tsn_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t new_cumulative_tsn;
	/* stream/sequence pairs (sctp_strseq) follow */
} __packed;

struct sctp_strseq {
	u_int16_t stream;
	u_int16_t sequence;
} __packed;

struct sctp_forward_tsn_msg {
	struct sctphdr sh;
	struct sctp_forward_tsn_chunk msg;
} __packed;

/* should be a multiple of 4 - 1 aka 3/7/11 etc. */

#define SCTP_NUM_DB_TO_VERIFY 3

struct sctp_chunk_desc {
	u_int8_t chunk_type;
	u_int8_t data_bytes[SCTP_NUM_DB_TO_VERIFY];
	u_int32_t tsn_ifany;
} __packed;


struct sctp_pktdrop_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t bottle_bw;
	u_int32_t current_onq;
	u_int16_t trunc_len;
	u_int16_t reserved;
	u_int8_t data[0];
} __packed;

#define SCTP_RESET_YOUR  0x01   /* reset your streams and send response */
#define SCTP_RESET_ALL   0x02   /* reset all of your streams */
#define SCTP_RECIPRICAL  0x04   /* reset my streams too */

struct sctp_stream_reset_request {
	struct sctp_paramhdr ph;
	u_int8_t reset_flags;		   /* actual request */
	u_int8_t reset_pad[3];
	u_int32_t reset_req_seq;           /* monotonically increasing seq no */
	u_int16_t list_of_streams[0];      /* if not all list of streams */
} __packed;

#define SCTP_RESET_PERFORMED        0x01   /* Peers sending str was reset */
#define SCTP_RESET_DENIED           0x02   /* Asked for but refused       */

struct sctp_stream_reset_response {
	struct sctp_paramhdr ph;
	u_int8_t reset_flags;		   /* actual request */
	u_int8_t reset_pad[3];
	u_int32_t reset_req_seq_resp;   /* copied from reset_req reset_req_seq */
	u_int32_t reset_at_tsn;		/* resetters next TSN to be assigned send wise */
	u_int32_t cumulative_tsn;	/* resetters cum-ack point receive wise */
	u_int16_t list_of_streams[0];   /* if not all list of streams */
} __packed;

/* convience structures, note that if you
 * are making a request for specific streams
 * then the request will need to be an overlay
 * structure.
 */

struct sctp_stream_reset_req {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_request sr_req;
} __packed;

struct sctp_stream_reset_resp {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_response sr_resp;
} __packed;


/*
 * we pre-reserve enough room for a ECNE or CWR AND a SACK with no
 * missing pieces. If ENCE is missing we could have a couple of blocks.
 * This way we optimize so we MOST likely can bundle a SACK/ECN with
 * the smallest size data chunk I will split into. We could increase
 * throughput slightly by taking out these two but the  24-sack/8-CWR
 * i.e. 32 bytes I pre-reserve I feel is worth it for now.
 */
#ifndef SCTP_MAX_OVERHEAD
#ifdef AF_INET6
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip6_hdr))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip6_hdr))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip6_hdr) + \
			   sizeof(struct sctphdr))

#else
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip) + \
			   sizeof(struct sctphdr))

#endif /* AF_INET6 */
#endif /* !SCTP_MAX_OVERHEAD */

#define SCTP_MED_V4_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			      sizeof(struct sctphdr) + \
			      sizeof(struct ip))

#define SCTP_MIN_V4_OVERHEAD (sizeof(struct ip) + \
			      sizeof(struct sctphdr))

#endif /* !__SCTP_HEADER_H__ */
