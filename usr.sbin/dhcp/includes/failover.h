/* failover.h

   Definitions for address trees... */

/*
 * Copyright (c) 1999 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#if defined (FAILOVER_PROTOCOL)
struct failover_option_info {
	int code;
	const char *name;
	enum { FT_UINT8, FT_IPADDR, FT_UINT32, FT_BYTES, FT_TEXT_OR_BYTES,
	       FT_DDNS, FT_DDNS1, FT_UINT16, FT_TEXT,
	       FT_UNDEF, FT_DIGEST } type;
	int num_present;
	int offset;
	u_int32_t bit;
};

typedef struct {
	unsigned count;
	u_int8_t *data;
} failover_option_t;

#define FM_OFFSET(x) (long)(&(((failover_message_t *)0) -> x))

/* Failover message options: */
#define FTO_BINDING_STATUS		1
#define FTB_BINDING_STATUS			0x00000002
#define FTO_ASSIGNED_IP_ADDRESS		2
#define FTB_ASSIGNED_IP_ADDRESS			0x00000004
#define FTO_SERVER_ADDR			3
#define FTB_SERVER_ADDR				0x00000008
#define FTO_ADDRESSES_TRANSFERRED	4
#define FTB_ADDRESSES_TRANSFERRED		0x00000010
#define FTO_CLIENT_IDENTIFIER		5
#define FTB_CLIENT_IDENTIFIER			0x00000020
#define FTO_CHADDR			6
#define FTB_CHADDR				0x00000040
#define FTO_DDNS			7
#define FTB_DDNS				0x00000080
#define FTO_REJECT_REASON		8
#define FTB_REJECT_REASON			0x00000100
#define FTO_MESSAGE			9
#define FTB_MESSAGE				0x00000200
#define FTO_MCLT			10
#define FTB_MCLT				0x00000400
#define FTO_VENDOR_CLASS		11
#define FTB_VENDOR_CLASS			0x00000800
#define FTO_LEASE_EXPIRY		13
#define FTB_LEASE_EXPIRY			0x00002000
#define FTO_POTENTIAL_EXPIRY		14
#define FTB_POTENTIAL_EXPIRY			0x00004000
#define FTO_GRACE_EXPIRY		15
#define FTB_GRACE_EXPIRY			0x00008000
#define FTO_CLTT			16
#define FTB_CLTT				0x00010000
#define FTO_STOS			17
#define FTB_STOS				0x00020000
#define FTO_SERVER_STATE		18
#define FTB_SERVER_STATE			0x00040000
#define FTO_SERVER_FLAGS		19
#define FTB_SERVER_FLAGS			0x00080000
#define FTO_VENDOR_OPTIONS		20
#define FTB_VENDOR_OPTIONS			0x00100000
#define FTO_MAX_UNACKED			21
#define FTB_MAX_UNACKED				0x00200000
#define FTO_RECEIVE_TIMER		23
#define FTB_RECEIVE_TIMER			0x00800000
#define FTO_HBA				24
#define FTB_HBA					0x01000000
#define FTO_MESSAGE_DIGEST		25
#define FTB_MESSAGE_DIGEST			0x02000000
#define FTO_PROTOCOL_VERSION		26
#define FTB_PROTOCOL_VERSION			0x04000000
#define FTO_TLS_REQUEST			27
#define FTB_TLS_REQUEST				0x08000000
#define FTO_TLS_REPLY			28
#define FTB_TLS_REPLY				0x10000000
#define FTO_REQUEST_OPTIONS		29
#define FTB_REQUEST_OPTIONS			0x20000000
#define FTO_REPLY_OPTIONS		30
#define FTB_REPLY_OPTIONS			0x40000000
#define FTO_MAX				FTO_REPLY_OPTIONS

/* Failover protocol message types: */
#define FTM_POOLREQ		1
#define FTM_POOLRESP		2
#define FTM_BNDUPD		3
#define FTM_BNDACK		4
#define FTM_CONNECT		5
#define FTM_CONNECTACK		6
#define FTM_UPDREQ		7
#define FTM_UPDDONE		8
#define FTM_UPDREQALL		9
#define FTM_STATE		10
#define FTM_CONTACT		11
#define FTM_DISCONNECT		12

/* Reject reasons: */

#define FTR_ILLEGAL_IP_ADDR	1
#define FTR_FATAL_CONFLICT	2
#define FTR_MISSING_BINDINFO	3
#define FTR_TIMEMISMATCH	4
#define FTR_INVALID_MCLT	5
#define FTR_MISC_REJECT		6
#define FTR_DUP_CONNECTION	7
#define FTR_INVALID_PARTNER	8
#define FTR_TLS_UNSUPPORTED	9
#define FTR_TLS_UNCONFIGURED	10
#define FTR_TLS_REQUIRED	11
#define FTR_DIGEST_UNSUPPORTED	12
#define FTR_DIGEST_UNCONFIGURED	13
#define FTR_VERSION_MISMATCH	14
#define FTR_MISSING_BIND_INFO	15
#define FTR_OUTDATED_BIND_INFO	16
#define FTR_LESS_CRIT_BIND_INFO	17
#define FTR_NO_TRAFFIC		18
#define FTR_HBA_CONFLICT	19
#define FTR_UNKNOWN		254

/* Lease states: */
typedef enum {
	FTS_FREE = 1,
	FTS_ACTIVE = 2,
	FTS_EXPIRED = 3,
	FTS_RELEASED = 4,
	FTS_ABANDONED = 5,
	FTS_RESET = 6,
	FTS_BACKUP = 7,
	FTS_RESERVED = 8,
	FTS_BOOTP = 9
} binding_state_t;

#define DHCP_FAILOVER_MAX_MESSAGE_SIZE	2048

typedef struct {
	u_int8_t type;

	u_int8_t binding_status;
	u_int8_t protocol_version;
	u_int8_t reject_reason;
	u_int8_t server_flags;
	u_int8_t server_state;
	u_int8_t tls_reply;
	u_int8_t tls_request;
	u_int32_t stos;
	u_int32_t time;
	u_int32_t xid;
	u_int32_t addresses_transferred;
	u_int32_t assigned_addr;
	u_int32_t client_ltt;
	u_int32_t expiry;
	u_int32_t grace_expiry;
	u_int32_t max_unacked;
	u_int32_t mclt;
	u_int32_t potential_expiry;
	u_int32_t receive_timer;
	u_int32_t server_addr;
	failover_option_t chaddr;
	failover_option_t client_identifier;
	failover_option_t hba;
	failover_option_t message;
	failover_option_t reply_options;
	failover_option_t request_options;
	ddns_fqdn_t ddns;
	failover_option_t vendor_class;
	failover_option_t vendor_options;

	int options_present;
} failover_message_t;

typedef struct {
	OMAPI_OBJECT_PREAMBLE;
	struct option_cache *peer_address;
	unsigned peer_port;
	int options_present;
	enum dhcp_flink_state {
		dhcp_flink_start,
		dhcp_flink_message_length_wait,
		dhcp_flink_message_wait,
		dhcp_flink_disconnected,
		dhcp_flink_state_max
	} state;
	failover_message_t *imsg;
	struct _dhcp_failover_state *state_object;
	u_int16_t imsg_len;
	unsigned imsg_count;
	u_int8_t imsg_payoff; /* Pay*load* offset. :') */
	u_int32_t xid;
} dhcp_failover_link_t;

typedef struct {
	OMAPI_OBJECT_PREAMBLE;
	unsigned local_port;
} dhcp_failover_listener_t;
#endif /* FAILOVER_PROTOCOL */

/* A failover peer. */
enum failover_state {
	unknown_state,
	partner_down,
	normal,
	communications_interrupted,
	potential_conflict_nic,
	potential_conflict,
	recover
};

#if defined (FAILOVER_PROTOCOL)
typedef struct _dhcp_failover_state {
	OMAPI_OBJECT_PREAMBLE;
	struct _dhcp_failover_state *next;
	char *name;			/* Name of this failover instance. */
	struct option_cache *address;	/* Partner's IP address or hostname. */
	int port;			/* Partner's TCP port. */
	struct option_cache *server_addr; /* IP address on which to listen. */
	struct data_string server_identifier; /* Server identifier (IP addr) */
	int listen_port;		/* Port on which to listen. */
	u_int32_t max_flying_updates;
	u_int32_t mclt;

	u_int8_t *hba;	/* Hash bucket array for load balancing. */
	int load_balance_max_secs;

	enum failover_state partner_state;
	TIME partner_stos;
	enum failover_state my_state;
	TIME my_stos;

	dhcp_failover_link_t *link_to_peer;	/* Currently-established link
						   to peer. */

	enum {
		primary, secondary
	} i_am;		/* We are primary or secondary in this relationship. */

	TIME last_packet_sent;		/* Timestamp on last packet we sent. */
	TIME last_timestamp_received;	/* The last timestamp we sent that
					   has been returned by our partner. */
	TIME skew;	/* The skew between our clock and our partner's. */
	u_int32_t max_transmit_idle; /* Always send a poll if we haven't sent
					some other packet more recently than
					this. */
	u_int32_t max_response_delay;	/* If the returned timestamp on the
					   last packet we received is older
					   than this, communications have been
					   interrupted. */
	struct lease *update_queue_head; /* List of leases we haven't sent
					    to peer. */
	struct lease *update_queue_tail;

	struct lease *ack_queue_head;	/* List of lease updates the peer
					   hasn't yet acked. */
	struct lease *ack_queue_tail;
	int cur_unacked_updates;	/* Number of updates we've sent
					   that have not yet been acked. */
} dhcp_failover_state_t;

#define DHCP_FAILOVER_VERSION		1
#endif /* FAILOVER_PROTOCOL */
