/*	$KAME: sctp_structs.h,v 1.13 2005/03/06 16:04:18 itojun Exp $	*/
/*	$NetBSD: sctp_structs.h,v 1.1 2015/10/13 21:28:35 rjs Exp $ */

#ifndef __SCTP_STRUCTS_H__
#define __SCTP_STRUCTS_H__

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
#include <sys/queue.h>

#include <sys/callout.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif

#include <netinet/sctp_header.h>
#include <netinet/sctp_uio.h>

struct sctp_timer {
	struct callout timer;
	int type;
	/*
	 * Depending on the timer type these will be setup and cast with
	 * the appropriate entity.
	 */
	void *ep;
	void *tcb;
	void *net;
};

/*
 * This is the information we track on each interface that we know about	* from the distant end.
 */
TAILQ_HEAD(sctpnetlisthead, sctp_nets);

/*
 * Users of the iterator need to malloc a iterator with a call to
 * sctp_initiate_iterator(func, pcb_flags, asoc_state, void-ptr-arg, u_int32_t,
 *                        u_int32-arg, end_func, inp);
 *
 * Use the following two defines if you don't care what pcb flags are on the
 * EP and/or you don't care what state the association is in.
 *
 * Note that if you specify an INP as the last argument then ONLY each
 * association of that single INP will be executed upon. Note that the
 * pcb flags STILL apply so if the inp you specify has different pcb_flags
 * then what you put in pcb_flags nothing will happen. use SCTP_PCB_ANY_FLAGS
 * to assure the inp you specify gets treated.
 */
#define SCTP_PCB_ANY_FLAGS  0x00000000
#define SCTP_ASOC_ANY_STATE 0x00000000

typedef void (*asoc_func)(struct sctp_inpcb *, struct sctp_tcb *, void *ptr,
			  u_int32_t val);
typedef void (*end_func)(void *ptr, u_int32_t val);

#define SCTP_ITERATOR_DO_ALL_INP	0x00000001
#define SCTP_ITERATOR_DO_SINGLE_INP	0x00000002

struct sctp_iterator {
        LIST_ENTRY(sctp_iterator) sctp_nxt_itr;
	struct sctp_timer tmr;
	struct sctp_inpcb *inp;	/* ep */
	struct sctp_tcb *stcb;	/* assoc */
	asoc_func function_toapply;
	end_func function_atend;
	void *pointer;		/* pointer for apply func to use */
	u_int32_t val;		/* value for apply func to use */
	u_int32_t pcb_flags;
	u_int32_t asoc_state;
	u_int32_t iterator_flags;
};

LIST_HEAD(sctpiterators, sctp_iterator);

struct sctp_copy_all {
	struct sctp_inpcb *inp;	/* ep */
	struct mbuf *m;
	struct sctp_sndrcvinfo sndrcv;
	int sndlen;
	int cnt_sent;
	int cnt_failed;
};

union sctp_sockstore {
#ifdef AF_INET
	struct sockaddr_in  sin;
#endif
#ifdef AF_INET6
	struct sockaddr_in6 sin6;
#endif
	struct sockaddr     sa;
};

struct sctp_nets {
	TAILQ_ENTRY(sctp_nets) sctp_next;	/* next link */

        /* Things on the top half may be able to be split
	 * into a common structure shared by all.
	 */
	struct sctp_timer pmtu_timer;

	/*
	 * The following two in combination equate to a route entry for
	 * v6 or v4.
	 */
#if 0
	struct sctp_route {
		struct rtentry *ro_rt;
		union sctp_sockstore _l_addr;	/* remote peer addr */
		union sctp_sockstore _s_addr;	/* our selected src addr */
	} ro;
#endif
	struct route ro;
	/* union sctp_sockstore _l_addr; */
	union sctp_sockstore _s_addr;
	/* mtu discovered so far */
	u_int32_t mtu;
        u_int32_t ssthresh;		/* not sure about this one for split */

	/* smoothed average things for RTT and RTO itself */
	int lastsa;
	int lastsv;
	unsigned int RTO;

	/* This is used for SHUTDOWN/SHUTDOWN-ACK/SEND or INIT timers */
	struct sctp_timer rxt_timer;

	/* last time in seconds I sent to it */
	struct timeval last_sent_time;
	int ref_count;

	/* Congestion stats per destination */
	/*
	 * flight size variables and such, sorry Vern, I could not avoid
	 * this if I wanted performance :>
	 */
	u_int32_t flight_size;
	u_int32_t cwnd; /* actual cwnd */
	u_int32_t prev_cwnd; /* cwnd before any processing */
	u_int32_t partial_bytes_acked; /* in CA tracks when to incr a MTU */

	/* tracking variables to avoid the aloc/free in sack processing */
	unsigned int net_ack;
	unsigned int net_ack2;
	/*
	 * These only are valid if the primary dest_sstate holds the
	 * SCTP_ADDR_SWITCH_PRIMARY flag
	 */
	u_int32_t next_tsn_at_change;
	u_int32_t heartbeat_random1;
	u_int32_t heartbeat_random2;

	/* if this guy is ok or not ... status */
	u_int16_t dest_state;
	/* number of transmit failures to down this guy */
	u_int16_t failure_threshold;
	/* error stats on destination */
	u_int16_t error_count;

	/* Flags that probably can be combined into dest_state */
	u_int8_t rto_pending;		/* is segment marked for RTO update  ** if we split?*/
	u_int8_t fast_retran_ip;	/* fast retransmit in progress */
	u_int8_t hb_responded;
	u_int8_t cacc_saw_newack;	/* CACC algorithm flag */
        u_int8_t src_addr_selected;	/* if we split we move */
	u_int8_t indx_of_eligible_next_to_use;
	u_int8_t addr_is_local;		/* its a local address (if known) could move in split */
#ifdef SCTP_HIGH_SPEED
	u_int8_t last_hs_used;		/* index into the last HS table entry we used */
#endif
};


struct sctp_data_chunkrec {
	u_int32_t TSN_seq;  /* the TSN of this transmit */
	u_int16_t stream_seq; /* the stream sequence number of this transmit */
	u_int16_t stream_number; /* the stream number of this guy */
	u_int32_t payloadtype;
	u_int32_t context;	/* from send */

       /* ECN Nonce: Nonce Value for this chunk */
        u_int8_t ect_nonce;

	/* part of the Highest sacked algorithm to be able to
	 * stroke counts on ones that are FR'd.
	 */
	u_int32_t fast_retran_tsn;	/* sending_seq at the time of FR */
	struct timeval timetodrop;	/* time we drop it from queue */
	u_int8_t doing_fast_retransmit;
	u_int8_t rcv_flags; /* flags pulled from data chunk on inbound
			   * for outbound holds sending flags.
			   */
	u_int8_t state_flags;
};

TAILQ_HEAD(sctpchunk_listhead, sctp_tmit_chunk);

#define CHUNK_FLAGS_FRAGMENT_OK	0x0001

struct sctp_tmit_chunk {
	union {
		struct sctp_data_chunkrec data;
		int chunk_id;
	} rec;
	int32_t   sent;		/* the send status */
	int32_t   snd_count;			/* number of times I sent */
	u_int32_t flags;		/* flags, such as FRAGMENT_OK */
	u_int32_t   send_size;
	u_int32_t   book_size;
	u_int32_t   mbcnt;
	struct sctp_association *asoc;	/* bp to asoc this belongs to */
	struct timeval sent_rcv_time;	/* filled in if RTT being calculated */
	struct mbuf *data;		/* pointer to mbuf chain of data */
	struct sctp_nets *whoTo;
	TAILQ_ENTRY(sctp_tmit_chunk) sctp_next;	/* next link */
	uint8_t do_rtt;
};


/*
 * this struct contains info that is used to track inbound stream data
 * and help with ordering.
 */
TAILQ_HEAD(sctpwheelunrel_listhead, sctp_stream_in);
struct sctp_stream_in {
	struct sctpchunk_listhead inqueue;
	TAILQ_ENTRY(sctp_stream_in) next_spoke;
	uint16_t stream_no;
	uint16_t last_sequence_delivered;	/* used for re-order */
};

/* This struct is used to track the traffic on outbound streams */
TAILQ_HEAD(sctpwheel_listhead, sctp_stream_out);
struct sctp_stream_out {
	struct sctpchunk_listhead outqueue;
	TAILQ_ENTRY(sctp_stream_out) next_spoke; /* next link in wheel */
	uint16_t stream_no;
	uint16_t next_sequence_sent; /* next one I expect to send out */
};

/* used to keep track of the addresses yet to try to add/delete */
TAILQ_HEAD(sctp_asconf_addrhead, sctp_asconf_addr);
struct sctp_asconf_addr {
	TAILQ_ENTRY(sctp_asconf_addr) next;
	struct sctp_asconf_addr_param ap;
	struct ifaddr *ifa;	/* save the ifa for add/del ip */
	uint8_t	sent;		/* has this been sent yet? */
};


/*
 * Here we have information about each individual association that we
 * track. We probably in production would be more dynamic. But for ease
 * of implementation we will have a fixed array that we hunt for in a
 * linear fashion.
 */
struct sctp_association {
	/* association state */
	int state;
	/* queue of pending addrs to add/delete */
	struct sctp_asconf_addrhead asconf_queue;
	struct timeval time_entered;		/* time we entered state */
	struct timeval time_last_rcvd;
	struct timeval time_last_sent;
	struct timeval time_last_sat_advance;
	struct sctp_sndrcvinfo def_send;	/* default send parameters */

	/* timers and such */
	struct sctp_timer hb_timer;		/* hb timer */
	struct sctp_timer dack_timer;		/* Delayed ack timer */
	struct sctp_timer asconf_timer;		/* Asconf */
	struct sctp_timer strreset_timer;	/* stream reset */
	struct sctp_timer shut_guard_timer;	/* guard */
	struct sctp_timer autoclose_timer;	/* automatic close timer */
	struct sctp_timer delayed_event_timer;	/* timer for delayed events */

	/* list of local addresses when add/del in progress */
	struct sctpladdr sctp_local_addr_list;
	struct sctpnetlisthead nets;

	/* Control chunk queue */
	struct sctpchunk_listhead control_send_queue;

	/* Once a TSN hits the wire it is moved to the sent_queue. We
	 * maintain two counts here (don't know if any but retran_cnt
	 * is needed). The idea is that the sent_queue_retran_cnt
	 * reflects how many chunks have been marked for retranmission
	 * by either T3-rxt or FR.
	 */
	struct sctpchunk_listhead sent_queue;
	struct sctpchunk_listhead send_queue;


	/* re-assembly queue for fragmented chunks on the inbound path */
	struct sctpchunk_listhead reasmqueue;

	/*
	 * this queue is used when we reach a condition that we can NOT
	 * put data into the socket buffer. We track the size of this
	 * queue and set our rwnd to the space in the socket minus also
	 * the size_on_delivery_queue.
	 */
	struct sctpchunk_listhead delivery_queue;

	struct sctpwheel_listhead out_wheel;

	/* If an iterator is looking at me, this is it */
	struct sctp_iterator *stcb_starting_point_for_iterator;

	/* ASCONF destination address last sent to */
	struct sctp_nets *asconf_last_sent_to;

	/* ASCONF save the last ASCONF-ACK so we can resend it if necessary */
	struct mbuf *last_asconf_ack_sent;

	/*
	 * if Source Address Selection happening, this will rotate through
	 * the link list.
	 */
	struct sctp_laddr *last_used_address;

	/* stream arrays */
        struct sctp_stream_in  *strmin;
	struct sctp_stream_out *strmout;
	u_int8_t *mapping_array;
	/* primary destination to use */
	struct sctp_nets *primary_destination;

	/* last place I got a data chunk from */
	struct sctp_nets *last_data_chunk_from;
	/* last place I got a control from */
	struct sctp_nets *last_control_chunk_from;

	/* circular looking for output selection */
	struct sctp_stream_out *last_out_stream;

	/* wait to the point the cum-ack passes
	 * pending_reply->sr_resp.reset_at_tsn.
	 */
	struct sctp_stream_reset_response *pending_reply;
	struct sctpchunk_listhead pending_reply_queue;

	u_int32_t cookie_preserve_req;
	/* ASCONF next seq I am sending out, inits at init-tsn */
	uint32_t asconf_seq_out;
	/* ASCONF last received ASCONF from peer, starts at peer's TSN-1 */
	uint32_t asconf_seq_in;

	/* next seq I am sending in str reset messages */
	uint32_t str_reset_seq_out;

	/* next seq I am expecting in str reset messages */
	uint32_t str_reset_seq_in;
	u_int32_t str_reset_sending_seq;

	/* various verification tag information */
	u_int32_t my_vtag;	/*
				 * The tag to be used. if assoc is
				 * re-initited by remote end, and
				 * I have unlocked this will be
				 * regenrated to a new random value.
				 */
	u_int32_t peer_vtag;	/* The peers last tag */

	u_int32_t my_vtag_nonce;
	u_int32_t peer_vtag_nonce;


	/* This is the SCTP fragmentation threshold */
	u_int32_t smallest_mtu;

	/*
	 * Special hook for Fast retransmit, allows us to track the highest
	 * TSN that is NEW in this SACK if gap ack blocks are present.
	 */
	u_int32_t this_sack_highest_gap;

	/*
	 * The highest consecutive TSN that has been acked by peer on my
	 * sends
	 */
	u_int32_t last_acked_seq;

	/* The next TSN that I will use in sending. */
	u_int32_t sending_seq;

	/* Original seq number I used ??questionable to keep?? */
	u_int32_t init_seq_number;

	/*
	 * We use this value to know if FR's are allowed, i.e. did the
	 * cum-ack pass this point or equal it so FR's are now allowed.
	 */
	u_int32_t t3timeout_highest_marked;

	/* The Advanced Peer Ack Point, as required by the PR-SCTP */
	/* (A1 in Section 4.2) */
	u_int32_t advanced_peer_ack_point;

	/*
	 * The highest consequetive TSN at the bottom of the mapping
	 * array (for his sends).
	 */
	u_int32_t cumulative_tsn;
	/*
	 * Used to track the mapping array and its offset bits. This
	 * MAY be lower then cumulative_tsn.
	 */
	u_int32_t mapping_array_base_tsn;
	/*
	 * used to track highest TSN we have received and is listed in
	 * the mapping array.
	 */
	u_int32_t highest_tsn_inside_map;

	u_int32_t last_echo_tsn;
	u_int32_t last_cwr_tsn;
	u_int32_t fast_recovery_tsn;
	u_int32_t sat_t3_recovery_tsn;

	u_int32_t tsn_last_delivered;

	/*
	 * window state information and smallest MTU that I use to bound
	 * segmentation
	 */
	u_int32_t peers_rwnd;
	u_int32_t my_rwnd;
	u_int32_t my_last_reported_rwnd;
	u_int32_t my_rwnd_control_len;

	u_int32_t total_output_queue_size;
	u_int32_t total_output_mbuf_queue_size;

	/* 32 bit nonce stuff */
	u_int32_t nonce_resync_tsn;
	u_int32_t nonce_wait_tsn;

	int ctrl_queue_cnt; /* could be removed  REM */
	/*
	 * All outbound datagrams queue into this list from the
	 * individual stream queue. Here they get assigned a TSN
	 * and then await sending. The stream seq comes when it
	 * is first put in the individual str queue
	 */
	unsigned int stream_queue_cnt;
	unsigned int send_queue_cnt;
	unsigned int sent_queue_cnt;
	unsigned int sent_queue_cnt_removeable;
	/*
	 * Number on sent queue that are marked for retran until this
	 * value is 0 we only send one packet of retran'ed data.
	 */
	unsigned int sent_queue_retran_cnt;

	unsigned int size_on_reasm_queue;
	unsigned int cnt_on_reasm_queue;
	/* amount of data (bytes) currently in flight (on all destinations) */
	unsigned int total_flight;
	/* Total book size in flight */
	unsigned int total_flight_count;	/* count of chunks used with book total */
	/* count of destinaton nets and list of destination nets */
	unsigned int numnets;

	/* Total error count on this association */
	unsigned int overall_error_count;

	unsigned int size_on_delivery_queue;
	unsigned int cnt_on_delivery_queue;

	unsigned int cnt_msg_on_sb;

	/* All stream count of chunks for delivery */
	unsigned int size_on_all_streams;
	unsigned int cnt_on_all_streams;

	/* Heart Beat delay in ticks */
	unsigned int heart_beat_delay;

	/* autoclose */
	unsigned int sctp_autoclose_ticks;

	/* how many preopen streams we have */
	unsigned int pre_open_streams;

	/* How many streams I support coming into me */
	unsigned int max_inbound_streams;

	/* the cookie life I award for any cookie, in seconds */
	unsigned int cookie_life;

	unsigned int numduptsns;
	int dup_tsns[SCTP_MAX_DUP_TSNS];
	unsigned int initial_init_rto_max;	/* initial RTO for INIT's */
	unsigned int initial_rto;		/* initial send RTO */
	unsigned int minrto;			/* per assoc RTO-MIN */
	unsigned int maxrto;			/* per assoc RTO-MAX */
	/* Being that we have no bag to collect stale cookies, and
	 * that we really would not want to anyway.. we will count
	 * them in this counter. We of course feed them to the
	 * pigeons right away (I have always thought of pigeons
	 * as flying rats).
	 */
	u_int16_t stale_cookie_count;

	/* For the partial delivery API, if up, invoked
	 * this is what last TSN I delivered
	 */
	u_int16_t str_of_pdapi;
	u_int16_t ssn_of_pdapi;


	/* counts of actual built streams. Allocation may be more however */
	/* could re-arrange to optimize space here. */
	u_int16_t streamincnt;
	u_int16_t streamoutcnt;

	/* my maximum number of retrans of INIT and SEND */
	/* copied from SCTP but should be individually setable */
	u_int16_t max_init_times;
	u_int16_t max_send_times;

	u_int16_t def_net_failure;

	/*
	 * lock flag: 0 is ok to send, 1+ (duals as a retran count) is
	 * awaiting ACK
	 */
	u_int16_t asconf_sent;   /* possibly removable REM */
	u_int16_t mapping_array_size;

	u_int16_t chunks_on_out_queue; /* total chunks floating around */
	int16_t num_send_timers_up;
	/*
	 * This flag indicates that we need to send the first SACK. If
	 * in place it says we have NOT yet sent a SACK and need to.
	 */
	u_int8_t first_ack_sent;

	/* max burst after fast retransmit completes */
	u_int8_t max_burst;

	u_int8_t sat_network;	/* RTT is in range of sat net or greater */
	u_int8_t sat_network_lockout;/* lockout code */
	u_int8_t burst_limit_applied;	/* Burst limit in effect at last send? */
	/* flag goes on when we are doing a partial delivery api */
	u_int8_t hb_random_values[4];
	u_int8_t fragmented_delivery_inprogress;
	u_int8_t fragment_flags;
	u_int8_t hb_ect_randombit;
        u_int8_t hb_random_idx;

	/* ECN Nonce stuff */
	u_int8_t receiver_nonce_sum; /* nonce I sum and put in my sack */
	u_int8_t ecn_nonce_allowed;  /* Tells us if ECN nonce is on */
	u_int8_t nonce_sum_check;    /* On off switch used during re-sync */
	u_int8_t nonce_wait_for_ecne;/* flag when we expect a ECN */
	u_int8_t peer_supports_ecn_nonce;

	/*
	 * This value, plus all other ack'd but above cum-ack is added
	 * together to cross check against the bit that we have yet to
	 * define (probably in the SACK).
	 * When the cum-ack is updated, this sum is updated as well.
	 */
	u_int8_t nonce_sum_expect_base;
	/* Flag to tell if ECN is allowed */
	u_int8_t ecn_allowed;

	/* flag to indicate if peer can do asconf */
	uint8_t peer_supports_asconf;
	uint8_t peer_supports_asconf_setprim; /* possibly removable REM */
	/* pr-sctp support flag */
	uint8_t peer_supports_prsctp;

	/* stream resets are supported by the peer */
	uint8_t peer_supports_strreset;

	/*
	 * packet drop's are supported by the peer, we don't really care
	 * about this but we bookkeep it anyway.
	 */
	uint8_t peer_supports_pktdrop;

	/* Do we allow V6/V4? */
	u_int8_t ipv4_addr_legal;
	u_int8_t ipv6_addr_legal;
	/* Address scoping flags */
	/* scope value for IPv4 */
	u_int8_t ipv4_local_scope;
	/* scope values for IPv6 */
	u_int8_t local_scope;
	u_int8_t site_scope;
	/* loopback scope */
	u_int8_t loopback_scope;
	/* flags to handle send alternate net tracking */
	u_int8_t used_alt_onsack;
	u_int8_t used_alt_asconfack;
	u_int8_t fast_retran_loss_recovery;
	u_int8_t sat_t3_loss_recovery;
        u_int8_t dropped_special_cnt;
	u_int8_t seen_a_sack_this_pkt;
	u_int8_t stream_reset_outstanding;
	u_int8_t delayed_connection;
	u_int8_t ifp_had_enobuf;
	u_int8_t saw_sack_with_frags;
	/*
	 * The mapping array is used to track out of order sequences above
	 * last_acked_seq. 0 indicates packet missing 1 indicates packet
	 * rec'd. We slide it up every time we raise last_acked_seq and 0
	 * trailing locactions out.  If I get a TSN above the array
	 * mappingArraySz, I discard the datagram and let retransmit happen.
	 */
};

#endif
