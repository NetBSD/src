/*	$KAME: sctp_pcb.h,v 1.21 2005/07/16 01:18:47 suz Exp $	*/
/*	$NetBSD: sctp_pcb.h,v 1.1.18.2 2017/12/03 11:39:04 jdolecek Exp $ */

#ifndef __SCTP_PCB_H__
#define __SCTP_PCB_H__

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

/*
 * We must have V6 so the size of the proto can be calculated. Otherwise
 * we would not allocate enough for Net/Open BSD :-<
 */
#include <net/if.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6protosw.h>
#include <netinet6/in6_var.h>
#include <netinet6/in6_pcb.h>

#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>

LIST_HEAD(sctppcbhead, sctp_inpcb);
LIST_HEAD(sctpasochead, sctp_tcb);
TAILQ_HEAD(sctpsocketq, sctp_socket_q_list);
LIST_HEAD(sctpladdr, sctp_laddr);
LIST_HEAD(sctpvtaghead, sctp_tagblock);

#include <netinet/sctp_structs.h>
#include <netinet/sctp_uio.h>

/*
 * PCB flags
 */
#define SCTP_PCB_FLAGS_UDPTYPE		0x00000001
#define SCTP_PCB_FLAGS_TCPTYPE		0x00000002
#define SCTP_PCB_FLAGS_BOUNDALL		0x00000004
#define SCTP_PCB_FLAGS_ACCEPTING	0x00000008
#define SCTP_PCB_FLAGS_UNBOUND		0x00000010
#define SCTP_PCB_FLAGS_DO_ASCONF	0x00000020
#define SCTP_PCB_FLAGS_AUTO_ASCONF	0x00000040
/* socket options */
#define SCTP_PCB_FLAGS_NODELAY		0x00000100
#define SCTP_PCB_FLAGS_AUTOCLOSE	0x00000200
#define SCTP_PCB_FLAGS_RECVDATAIOEVNT	0x00000400
#define SCTP_PCB_FLAGS_RECVASSOCEVNT	0x00000800
#define SCTP_PCB_FLAGS_RECVPADDREVNT	0x00001000
#define SCTP_PCB_FLAGS_RECVPEERERR	0x00002000
#define SCTP_PCB_FLAGS_RECVSENDFAILEVNT	0x00004000
#define SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT	0x00008000
#define SCTP_PCB_FLAGS_ADAPTIONEVNT	0x00010000
#define SCTP_PCB_FLAGS_PDAPIEVNT	0x00020000
#define SCTP_PCB_FLAGS_STREAM_RESETEVNT 0x00040000
#define SCTP_PCB_FLAGS_NO_FRAGMENT	0x00080000
/* TCP model support */
#define SCTP_PCB_FLAGS_CONNECTED	0x00100000
#define SCTP_PCB_FLAGS_IN_TCPPOOL	0x00200000
#define SCTP_PCB_FLAGS_DONT_WAKE	0x00400000
#define SCTP_PCB_FLAGS_WAKEOUTPUT	0x00800000
#define SCTP_PCB_FLAGS_WAKEINPUT	0x01000000
#define SCTP_PCB_FLAGS_BOUND_V6		0x02000000
#define SCTP_PCB_FLAGS_NEEDS_MAPPED_V4	0x04000000
#define SCTP_PCB_FLAGS_BLOCKING_IO	0x08000000
#define SCTP_PCB_FLAGS_SOCKET_GONE	0x10000000
#define SCTP_PCB_FLAGS_SOCKET_ALLGONE	0x20000000

/* flags to copy to new PCB */
#define SCTP_PCB_COPY_FLAGS		0x0707ff64

#define SCTP_PCBHASH_ALLADDR(port, mask) (port & mask)
#define SCTP_PCBHASH_ASOC(tag, mask) (tag & mask)

struct sctp_laddr {
	LIST_ENTRY(sctp_laddr) sctp_nxt_addr;	/* next in list */
	struct ifaddr *ifa;
};

struct sctp_timewait {
	uint32_t tv_sec_at_expire;	/* the seconds from boot to expire */
	uint32_t v_tag;		/* the vtag that can not be reused */
};

struct sctp_tagblock {
        LIST_ENTRY(sctp_tagblock) sctp_nxt_tagblock;
	struct sctp_timewait vtag_block[SCTP_NUMBER_IN_VTAG_BLOCK];
};

struct sctp_epinfo {
	struct sctpasochead *sctp_asochash;
	u_long hashasocmark;

	struct sctppcbhead *sctp_ephash;
	u_long hashmark;

	/*
	 * The TCP model represents a substantial overhead in that we get
	 * an additional hash table to keep explicit connections in. The
	 * listening TCP endpoint will exist in the usual ephash above and
	 * accept only INIT's. It will be incapable of sending off an INIT.
	 * When a dg arrives we must look in the normal ephash. If we find
	 * a TCP endpoint that will tell us to go to the specific endpoint
	 * hash and re-hash to find the right assoc/socket. If we find a
	 * UDP model socket we then must complete the lookup. If this fails,
	 * i.e. no association can be found then we must continue to see if
	 * a sctp_peeloff()'d socket is in the tcpephash (a spun off socket
	 * acts like a TCP model connected socket).
	 */
	struct sctppcbhead *sctp_tcpephash;
	u_long hashtcpmark;
	uint32_t hashtblsize;

	struct sctppcbhead listhead;

	struct sctpiterators iteratorhead;

	/* ep zone info */
#if defined(__FreeBSD__) || defined(__APPLE__)
#if __FreeBSD_version >= 500000
	struct uma_zone *ipi_zone_ep;
	struct uma_zone *ipi_zone_asoc;
	struct uma_zone *ipi_zone_laddr;
	struct uma_zone *ipi_zone_net;
	struct uma_zone *ipi_zone_chunk;
	struct uma_zone *ipi_zone_sockq;
#else
	struct vm_zone *ipi_zone_ep;
	struct vm_zone *ipi_zone_asoc;
	struct vm_zone *ipi_zone_laddr;
	struct vm_zone *ipi_zone_net;
	struct vm_zone *ipi_zone_chunk;
	struct vm_zone *ipi_zone_sockq;
#endif
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct pool ipi_zone_ep;
	struct pool ipi_zone_asoc;
	struct pool ipi_zone_laddr;
	struct pool ipi_zone_net;
	struct pool ipi_zone_chunk;
	struct pool ipi_zone_sockq;
	struct pool ipi_zone_hash;
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx ipi_ep_mtx;
	struct mtx it_mtx;
#elif 0 /* defined(__NetBSD__) */
	krwlock_t ipi_ep_mtx;
	kmutex_t it_mtx;
#endif
	u_int ipi_count_ep;
	u_quad_t ipi_gencnt_ep;

	/* assoc/tcb zone info */
	u_int ipi_count_asoc;
	u_quad_t ipi_gencnt_asoc;

	/* local addrlist zone info */
	u_int ipi_count_laddr;
	u_quad_t ipi_gencnt_laddr;

	/* remote addrlist zone info */
	u_int ipi_count_raddr;
	u_quad_t ipi_gencnt_raddr;

	/* chunk structure list for output */
	u_int ipi_count_chunk;
	u_quad_t ipi_gencnt_chunk;

	/* socket queue zone info */
	u_int ipi_count_sockq;
	u_quad_t ipi_gencnt_sockq;

	struct sctpvtaghead vtag_timewait[SCTP_STACK_VTAG_HASH_SIZE];

#ifdef _SCTP_NEEDS_CALLOUT_
	struct calloutlist callqueue;
#endif /* _SCTP_NEEDS_CALLOUT_ */

	uint32_t mbuf_track;

	/* for port allocations */
	uint16_t lastport;
	uint16_t lastlow;
	uint16_t lasthi;

};

extern uint32_t sctp_pegs[SCTP_NUMBER_OF_PEGS];
/*
 * Here we have all the relevant information for each SCTP entity created.
 * We will need to modify this as approprate. We also need to figure out
 * how to access /dev/random.
 */
struct sctp_pcb {
	unsigned int time_of_secret_change; /* number of seconds from timeval.tv_sec */
	uint32_t secret_key[SCTP_HOW_MANY_SECRETS][SCTP_NUMBER_OF_SECRETS];
	unsigned int size_of_a_cookie;

	unsigned int sctp_timeoutticks[SCTP_NUM_TMRS];
	unsigned int sctp_minrto;
	unsigned int sctp_maxrto;
	unsigned int initial_rto;

	int initial_init_rto_max;

	uint32_t sctp_sws_sender;
	uint32_t sctp_sws_receiver;

	/* various thresholds */
	/* Max times I will init at a guy */
	uint16_t max_init_times;

	/* Max times I will send before we consider someone dead */
	uint16_t max_send_times;

	uint16_t def_net_failure;

	/* number of streams to pre-open on a association */
	uint16_t pre_open_stream_count;
	uint16_t max_open_streams_intome;

	/* random number generator */
	uint32_t random_counter;
	uint8_t random_numbers[SCTP_SIGNATURE_ALOC_SIZE];
	uint8_t random_store[SCTP_SIGNATURE_ALOC_SIZE];

	/*
	 * This timer is kept running per endpoint.  When it fires it
	 * will change the secret key.  The default is once a hour
	 */
	struct sctp_timer signature_change;
	int def_cookie_life;
	/* defaults to 0 */
	int auto_close_time;
	uint32_t initial_sequence_debug;
	uint32_t adaption_layer_indicator;
	char store_at;
	uint8_t max_burst;
	char current_secret_number;
	char last_secret_number;
};

#ifndef SCTP_ALIGNMENT
#define SCTP_ALIGNMENT 32
#endif

#ifndef SCTP_ALIGNM1
#define SCTP_ALIGNM1 (SCTP_ALIGNMENT-1)
#endif

#define sctp_lport ip_inp.inp.inp_lport

struct sctp_socket_q_list {
	struct sctp_tcb *tcb;
	TAILQ_ENTRY(sctp_socket_q_list) next_sq;
};

struct sctp_inpcb {
	/*
	 * put an inpcb in front of it all, kind of a waste but we need
	 * to for compatability with all the other stuff.
	 */
	union {
		struct inpcb inp;
		char align[(sizeof(struct in6pcb) + SCTP_ALIGNM1) &
			  ~SCTP_ALIGNM1];
	} ip_inp;
	LIST_ENTRY(sctp_inpcb) sctp_list;	/* lists all endpoints */
	/* hash of all endpoints for model */
	LIST_ENTRY(sctp_inpcb) sctp_hash;

	/* count of local addresses bound, 0 if bound all */
	int laddr_count;
	/* list of addrs in use by the EP */
	struct sctpladdr sctp_addr_list;
	/* used for source address selection rotation */
	struct sctp_laddr *next_addr_touse;
	struct ifnet *next_ifn_touse;
	/* back pointer to our socket */
	struct socket *sctp_socket;
	uint32_t sctp_flags;			/* flag set */
	struct sctp_pcb sctp_ep;		/* SCTP ep data */
	/* head of the hash of all associations */
	struct sctpasochead *sctp_tcbhash;
	u_long sctp_hashmark;
	/* head of the list of all associations */
	struct sctpasochead sctp_asoc_list;
	/* queue of TCB's waiting to stuff data up the socket */
	struct sctpsocketq sctp_queue_list;
	void *sctp_tcb_at_block;
	struct sctp_iterator *inp_starting_point_for_iterator;
	int  error_on_block;
	uint32_t sctp_frag_point;
	uint32_t sctp_vtag_first;
	struct mbuf *pkt, *pkt_last, *sb_last_mpkt;
	struct mbuf *control;
#if !(defined(__FreeBSD__) || defined(__APPLE__))
#ifndef INP_IPV6
#define INP_IPV6	0x1
#endif
#ifndef INP_IPV4
#define INP_IPV4	0x2
#endif
	u_char inp_vflag;
	u_char inp_ip_ttl;
	u_char inp_ip_tos;
	u_char inp_ip_resv;
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx inp_mtx;
	struct mtx inp_create_mtx;
	u_int32_t refcount;
#elif defined(__NetBSD__)
	kmutex_t inp_mtx;
	kmutex_t inp_create_mtx;
	u_int32_t refcount;
#endif
};

struct sctp_tcb {
	struct socket *sctp_socket;		/* back pointer to socket */
	struct sctp_inpcb *sctp_ep;		/* back pointer to ep */
	LIST_ENTRY(sctp_tcb) sctp_tcbhash;	/* next link in hash table */
	LIST_ENTRY(sctp_tcb) sctp_tcblist;	/* list of all of the TCB's */
	LIST_ENTRY(sctp_tcb) sctp_asocs;
	struct sctp_association asoc;
	uint16_t rport;			/* remote port in network format */
	uint16_t resv;
#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx tcb_mtx;
#elif defined(__NetBSD__)
	kmutex_t tcb_mtx;
#endif
};

#if defined(__FreeBSD__) && __FreeBSD_version >= 503000

/* General locking concepts:
 * The goal of our locking is to of course provide
 * consistency and yet minimize overhead. We will
 * attempt to use non-recursive locks which are supposed
 * to be quite inexpensive. Now in order to do this the goal
 * is that most functions are not aware of locking. Once we
 * have a TCB we lock it and unlock when we are through. This
 * means that the TCB lock is kind-of a "global" lock when
 * working on an association. Caution must be used when
 * asserting a TCB_LOCK since if we recurse we deadlock.
 *
 * Most other locks (INP and INFO) attempt to localize
 * the locking i.e. we try to contain the lock and
 * unlock within the function that needs to lock it. This
 * sometimes mean we do extra locks and unlocks and loose
 * a bit of efficency, but if the performance statements about
 * non-recursive locks are true this should not be a problem.
 * One issue that arises with this only lock when needed
 * is that if an implicit association setup is done we
 * have a problem. If at the time I lookup an association
 * I have NULL in the tcb return, by the time I call to
 * create the association some other processor could
 * have created it. This is what the CREATE lock on
 * the endpoint. Places where we will be implicitly
 * creating the association OR just creating an association
 * (the connect call) will assert the CREATE_INP lock. This
 * will assure us that during all the lookup of INP and INFO
 * if another creator is also locking/looking up we can
 * gate the two to synchronize. So the CREATE_INP lock is
 * also another one we must use extreme caution in locking
 * to make sure we don't hit a re-entrancy issue.
 *
 * For non FreeBSD 5.x and above we provide a bunch
 * of EMPTY lock macro's so we can blatantly put locks
 * everywhere and they reduce to nothing on NetBSD/OpenBSD
 * and FreeBSD 4.x
 *
 */


/* When working with the global SCTP lists we lock and unlock
 * the INP_INFO lock. So when we go to lookup an association
 * we will want to do a SCTP_INP_INFO_RLOCK() and then when
 * we want to add a new association to the sctppcbinfo list's
 * we will do a SCTP_INP_INFO_WLOCK().
 */

/*
 * FIX ME, all locks right now have a
 * recursive check/panic to validate that I
 * don't have any lock recursion going on.
 */

#define SCTP_INP_INFO_LOCK_INIT() \
        mtx_init(&sctppcbinfo.ipi_ep_mtx, "sctp", "inp_info", MTX_DEF)

#ifdef xyzzy
#define SCTP_INP_INFO_RLOCK()	do { 					\
             if (mtx_owned(&sctppcbinfo.ipi_ep_mtx))                     \
		panic("INP INFO Recursive Lock-R");                     \
             mtx_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)

#define SCTP_INP_INFO_WLOCK()	do { 					\
             if (mtx_owned(&sctppcbinfo.ipi_ep_mtx))                     \
		panic("INP INFO Recursive Lock-W");                     \
             mtx_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)

#else

void SCTP_INP_INFO_RLOCK(void);
void SCTP_INP_INFO_WLOCK(void);

#endif

#define SCTP_INP_INFO_RUNLOCK()		mtx_unlock(&sctppcbinfo.ipi_ep_mtx)
#define SCTP_INP_INFO_WUNLOCK()		mtx_unlock(&sctppcbinfo.ipi_ep_mtx)

/* The INP locks we will use for locking an SCTP endpoint, so for
 * example if we want to change something at the endpoint level for
 * example random_store or cookie secrets we lock the INP level.
 */
#define SCTP_INP_LOCK_INIT(_inp) \
	mtx_init(&(_inp)->inp_mtx, "sctp", "inp", MTX_DEF | MTX_DUPOK)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	mtx_init(&(_inp)->inp_create_mtx, "sctp", "inp_create", \
		 MTX_DEF | MTX_DUPOK)

#define SCTP_INP_LOCK_DESTROY(_inp)	mtx_destroy(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)	mtx_destroy(&(_inp)->inp_create_mtx)

#ifdef xyzzy
#define SCTP_INP_RLOCK(_inp)	do { 					\
        struct sctp_tcb *xx_stcb;					\
        xx_stcb = LIST_FIRST(&_inp->sctp_asoc_list);                    \
        if (xx_stcb)                                                     \
              if (mtx_owned(&(xx_stcb)->tcb_mtx))                        \
                     panic("I own TCB lock?");                          \
        if (mtx_owned(&(_inp)->inp_mtx))                                 \
		panic("INP Recursive Lock-R");                          \
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
        struct sctp_tcb *xx_stcb;					\
        xx_stcb = LIST_FIRST(&_inp->sctp_asoc_list);                    \
        if (xx_stcb)                                                     \
              if (mtx_owned(&(xx_stcb)->tcb_mtx))                        \
                     panic("I own TCB lock?");                          \
        if (mtx_owned(&(_inp)->inp_mtx))                                 \
		panic("INP Recursive Lock-W");                          \
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#else
void SCTP_INP_RLOCK(struct sctp_inpcb *);
void SCTP_INP_WLOCK(struct sctp_inpcb *);

#endif


#define SCTP_INP_INCR_REF(_inp)        _inp->refcount++

#define SCTP_INP_DECR_REF(_inp)         do {                                 \
                                             if (_inp->refcount > 0)          \
                                                  _inp->refcount--;          \
                                             else                            \
                                                  panic("bad inp refcount"); \
}while (0)

#define SCTP_ASOC_CREATE_LOCK(_inp)  do {				\
        if (mtx_owned(&(_inp)->inp_create_mtx))                          \
		panic("INP Recursive CREATE");                          \
        mtx_lock(&(_inp)->inp_create_mtx);                              \
} while (0)

#define SCTP_INP_RUNLOCK(_inp)		mtx_unlock(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp)		mtx_unlock(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)	mtx_unlock(&(_inp)->inp_create_mtx)

/* For the majority of things (once we have found the association) we
 * will lock the actual association mutex. This will protect all
 * the assoiciation level queues and streams and such. We will
 * need to lock the socket layer when we stuff data up into
 * the receiving sb_mb. I.e. we will need to do an extra
 * SOCKBUF_LOCK(&so->so_rcv) even though the association is
 * locked.
 */

#define SCTP_TCB_LOCK_INIT(_tcb) \
	mutex_init(&(_tcb)->tcb_mtx, MUTEX_DEFAULT, IPL_NET)
#define SCTP_TCB_LOCK_DESTROY(_tcb)	mtx_destroy(&(_tcb)->tcb_mtx)
#define SCTP_TCB_LOCK(_tcb)  do {					\
        if (!mtx_owned(&(_tcb->sctp_ep->inp_mtx)))                       \
		panic("TCB locking and no INP lock");                   \
        if (mtx_owned(&(_tcb)->tcb_mtx))                                 \
		panic("TCB Lock-recursive");                            \
	mtx_lock(&(_tcb)->tcb_mtx);                                     \
} while (0)
#define SCTP_TCB_UNLOCK(_tcb)		mtx_unlock(&(_tcb)->tcb_mtx)

#define SCTP_ITERATOR_LOCK_INIT() \
        mtx_init(&sctppcbinfo.it_mtx, "sctp", "iterator", MTX_DEF)
#define SCTP_ITERATOR_LOCK()  do {					\
        if (mtx_owned(&sctppcbinfo.it_mtx))                              \
		panic("Iterator Lock");                                 \
	mtx_lock(&sctppcbinfo.it_mtx);                                  \
} while (0)

#define SCTP_ITERATOR_UNLOCK()	        mtx_unlock(&sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY()	mtx_destroy(&sctppcbinfo.it_mtx)
#elif 0 /* defined(__NetBSD__) */
#define SCTP_INP_INFO_LOCK_INIT() \
	rw_init(&sctppcbinfo.ipi_ep_mtx)

#define SCTP_INP_INFO_RLOCK()	do { 					\
		rw_enter(&sctppcbinfo.ipi_ep_mtx, RW_READER);           \
} while (0)

#define SCTP_INP_INFO_WLOCK()	do { 					\
             rw_enter(&sctppcbinfo.ipi_ep_mtx, RW_WRITER);              \
} while (0)

#define SCTP_INP_INFO_RUNLOCK()		rw_exit(&sctppcbinfo.ipi_ep_mtx)
#define SCTP_INP_INFO_WUNLOCK()		rw_exit(&sctppcbinfo.ipi_ep_mtx)

/* The INP locks we will use for locking an SCTP endpoint, so for
 * example if we want to change something at the endpoint level for
 * example random_store or cookie secrets we lock the INP level.
 */
#define SCTP_INP_LOCK_INIT(_inp) \
	mutex_init(&(_inp)->inp_mtx, MUTEX_DEFAULT, IPL_NET)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	mutex_init(&(_inp)->inp_create_mtx, MUTEX_DEFAULT, IPL_NET)

#define SCTP_INP_LOCK_DESTROY(_inp)	mutex_destroy(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)	mutex_destroy(&(_inp)->inp_create_mtx)

#define SCTP_INP_RLOCK(_inp)	do { 					\
	mutex_enter(&(_inp)->inp_mtx);                                  \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	mutex_enter(&(_inp)->inp_mtx);                                  \
} while (0)


#define SCTP_INP_INCR_REF(_inp) atomic_add_int(&((_inp)->refcount), 1)

#define SCTP_INP_DECR_REF(_inp) atomic_add_int(&((_inp)->refcount), -1)

#define SCTP_ASOC_CREATE_LOCK(_inp)  do {				\
        mutex_enter(&(_inp)->inp_create_mtx);                              \
} while (0)

#define SCTP_INP_RUNLOCK(_inp)		mutex_exit(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp)		mutex_exit(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)	mutex_exit(&(_inp)->inp_create_mtx)

/* For the majority of things (once we have found the association) we
 * will lock the actual association mutex. This will protect all
 * the assoiciation level queues and streams and such. We will
 * need to lock the socket layer when we stuff data up into
 * the receiving sb_mb. I.e. we will need to do an extra
 * SOCKBUF_LOCK(&so->so_rcv) even though the association is
 * locked.
 */

#define SCTP_TCB_LOCK_INIT(_tcb) \
	mutex_init(&(_tcb)->tcb_mtx, MUTEX_DEFAULT, IPL_NET)
#define SCTP_TCB_LOCK_DESTROY(_tcb)	mutex_destroy(&(_tcb)->tcb_mtx)
#define SCTP_TCB_LOCK(_tcb)  do {					\
	mutex_enter(&(_tcb)->tcb_mtx);                                     \
} while (0)
#define SCTP_TCB_UNLOCK(_tcb)		mutex_exit(&(_tcb)->tcb_mtx)

#define SCTP_ITERATOR_LOCK_INIT() \
        mutex_init(&sctppcbinfo.it_mtx, MUTEX_DEFAULT, IPL_NET)
#define SCTP_ITERATOR_LOCK()  do {					\
        if (mutex_owned(&sctppcbinfo.it_mtx))                           \
		panic("Iterator Lock");                                 \
	mutex_enter(&sctppcbinfo.it_mtx);                               \
} while (0)

#define SCTP_ITERATOR_UNLOCK()	        mutex_exit(&sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY()	mutex_destroy(&sctppcbinfo.it_mtx)
#else

/* Empty Lock declarations for all other
 * platforms pre-process away to nothing.
 */

/* Lock for INFO stuff */
#define SCTP_INP_INFO_LOCK_INIT()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_WLOCK()

#define SCTP_INP_INFO_RUNLOCK()
#define SCTP_INP_INFO_WUNLOCK()
/* Lock for INP */
#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)
#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)
#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)
#define SCTP_INP_WUNLOCK(_inp)
#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)
/* Lock for TCB */
#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_UNLOCK(_tcb)
/* iterator locks */
#define SCTP_ITERATOR_LOCK_INIT()
#define SCTP_ITERATOR_LOCK()
#define SCTP_ITERATOR_UNLOCK()
#define SCTP_ITERATOR_LOCK_DESTROY()
#endif

#if defined(_KERNEL)

extern struct sctp_epinfo sctppcbinfo;
extern int sctp_auto_asconf;

int SCTP6_ARE_ADDR_EQUAL(const struct in6_addr *a, const struct in6_addr *b);

void sctp_fill_pcbinfo(struct sctp_pcbinfo *);

struct sctp_nets *sctp_findnet(struct sctp_tcb *, struct sockaddr *);

struct sctp_inpcb *sctp_pcb_findep(struct sockaddr *, int, int);

int sctp_inpcb_bind(struct socket *, struct sockaddr *, struct lwp *);

struct sctp_tcb *sctp_findassociation_addr(struct mbuf *, int, int,
    struct sctphdr *, struct sctp_chunkhdr *, struct sctp_inpcb **,
    struct sctp_nets **);

struct sctp_tcb *sctp_findassociation_addr_sa(struct sockaddr *,
	struct sockaddr *, struct sctp_inpcb **, struct sctp_nets **, int);

void sctp_move_pcb_and_assoc(struct sctp_inpcb *, struct sctp_inpcb *,
	struct sctp_tcb *);

/*
 * For this call ep_addr, the to is the destination endpoint address
 * of the peer (relative to outbound). The from field is only used if
 * the TCP model is enabled and helps distingush amongst the subset
 * bound (non-boundall). The TCP model MAY change the actual ep field,
 * this is why it is passed.
 */
struct sctp_tcb *sctp_findassociation_ep_addr(struct sctp_inpcb **,
	struct sockaddr *, struct sctp_nets **, struct sockaddr *, struct sctp_tcb *);

struct sctp_tcb *sctp_findassociation_ep_asocid(struct sctp_inpcb *, vaddr_t);

struct sctp_tcb *sctp_findassociation_ep_asconf(struct mbuf *, int, int,
    struct sctphdr *, struct sctp_inpcb **, struct sctp_nets **);

int sctp_inpcb_alloc(struct socket *);


int sctp_is_address_on_local_host(struct sockaddr *addr);

void sctp_inpcb_free(struct sctp_inpcb *, int);

struct sctp_tcb *sctp_aloc_assoc(struct sctp_inpcb *, struct sockaddr *,
	int, int *, uint32_t);

void sctp_free_assoc(struct sctp_inpcb *, struct sctp_tcb *);

int sctp_add_local_addr_ep(struct sctp_inpcb *, struct ifaddr *);

int sctp_insert_laddr(struct sctpladdr *, struct ifaddr *);

void sctp_remove_laddr(struct sctp_laddr *);

int sctp_del_local_addr_ep(struct sctp_inpcb *, struct ifaddr *);

int sctp_del_local_addr_ep_sa(struct sctp_inpcb *, struct sockaddr *);

int sctp_add_remote_addr(struct sctp_tcb *, struct sockaddr *, int, int);

int sctp_del_remote_addr(struct sctp_tcb *, struct sockaddr *);

void sctp_pcb_init(void);

void sctp_free_remote_addr(struct sctp_nets *);

int sctp_add_local_addr_assoc(struct sctp_tcb *, struct ifaddr *);

int sctp_del_local_addr_assoc(struct sctp_tcb *, struct ifaddr *);

int sctp_del_local_addr_assoc_sa(struct sctp_tcb *, struct sockaddr *);

int sctp_load_addresses_from_init(struct sctp_tcb *, struct mbuf *, int, int,
    int, struct sctphdr *, struct sockaddr *);

int sctp_set_primary_addr(struct sctp_tcb *, struct sockaddr *, struct sctp_nets *);

int sctp_is_vtag_good(struct sctp_inpcb *, uint32_t, struct timeval *);

/*void sctp_drain(void);*/

int sctp_destination_is_reachable(struct sctp_tcb *, const struct sockaddr *);

int sctp_add_to_socket_q(struct sctp_inpcb *, struct sctp_tcb *);

struct sctp_tcb *sctp_remove_from_socket_q(struct sctp_inpcb *);


/* Null in last arg inpcb indicate run on ALL ep's. Specific
 * inp in last arg indicates run on ONLY assoc's of the
 * specified endpoint.
 */
int
sctp_initiate_iterator(asoc_func af, uint32_t, uint32_t, void *, uint32_t,
		       end_func ef, struct sctp_inpcb *);

extern void in6_sin6_2_sin (struct sockaddr_in *,
                            struct sockaddr_in6 *sin6);

#endif /* _KERNEL */
#endif /* !__SCTP_PCB_H__ */
