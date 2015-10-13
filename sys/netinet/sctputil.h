/*	$KAME: sctputil.h,v 1.15 2005/03/06 16:04:19 itojun Exp $	*/
/*	$NetBSD: sctputil.h,v 1.1 2015/10/13 21:28:35 rjs Exp $ */

#ifndef __SCTPUTIL_H__
#define __SCTPUTIL_H__

/*
 * Copyright (C) 2002, 2003, 2004 Cisco Systems Inc,
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(_KERNEL)

#ifdef SCTP_MBUF_DEBUG
#define sctp_m_freem(m) do { \
    printf("m_freem(%p) m->nxtpkt:%p at %s[%d]\n", \
	   (m), (m)->m_next, __FILE__, __LINE__); \
    m_freem(m); \
} while (0);
#else
#define sctp_m_freem m_freem
#endif

#define sctp_m_copym	m_copym

/*
 * Zone(pool) allocation routines: MUST be defined for each OS
 * zone = zone/pool pointer
 * name = string name of the zone/pool
 * size = size of each zone/pool element
 * number = number of elements in zone/pool
 */
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
#include <vm/uma.h>
#else
#include <vm/vm_zone.h>
#endif
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/pool.h>
#endif

/* SCTP_ZONE_INIT: initialize the zone */
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
#define UMA_ZFLAG_FULL	0x0020
#define SCTP_ZONE_INIT(zone, name, size, number) { \
	zone = uma_zcreate(name, size, NULL, NULL, NULL, NULL, UMA_ALIGN_PTR,\
		UMA_ZFLAG_FULL); \
	uma_zone_set_max(zone, number); \
}
#else
#define SCTP_ZONE_INIT(zone, name, size, number) \
	zone = zinit(name, size, number, ZONE_INTERRUPT, 0);
#endif
#elif defined(__APPLE__)
#define SCTP_ZONE_INIT(zone, name, size, number) \
	zone = (void *)zinit(size, number * size, number, name);
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#define SCTP_ZONE_INIT(zone, name, size, number) \
	pool_init(&(zone), size, 0, 0, 0, name, NULL, IPL_NET);
#else
	/* don't know this OS! */
	force_comile_error;
#endif

/* SCTP_ZONE_GET: allocate element from the zone */
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
#define SCTP_ZONE_GET(zone) \
	uma_zalloc(zone, M_NOWAIT);
#else
#define SCTP_ZONE_GET(zone) \
	zalloci(zone);
#endif
#elif defined(__APPLE__)
#define SCTP_ZONE_GET(zone) \
	zalloc(zone);
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#define SCTP_ZONE_GET(zone) \
	pool_get(&zone, PR_NOWAIT);
#else
	/* don't know this OS! */
	force_comile_error;
#endif

/* SCTP_ZONE_FREE: free element from the zone */
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
#define SCTP_ZONE_FREE(zone, element) \
	uma_zfree(zone, element);
#else
#define SCTP_ZONE_FREE(zone, element) \
	zfreei(zone, element);
#endif
#elif defined(__APPLE__)
#define SCTP_ZONE_FREE(zone, element) \
	zfree(zone, element);
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#define SCTP_ZONE_FREE(zone, element) \
	pool_put(&zone, element);
#else
	/* don't know this OS! */
	force_comile_error;
#endif

#define sctp_get_associd(stcb) ((sctp_assoc_t)stcb->asoc.my_vtag)

/*
 * Function prototypes
 */
struct ifaddr *sctp_find_ifa_by_addr(struct sockaddr *sa);

u_int32_t sctp_select_initial_TSN(struct sctp_pcb *);

u_int32_t sctp_select_a_tag(struct sctp_inpcb *);

int sctp_init_asoc(struct sctp_inpcb *, struct sctp_association *, int, uint32_t);

void sctp_fill_random_store(struct sctp_pcb *);

int sctp_timer_start(int, struct sctp_inpcb *, struct sctp_tcb *,
	struct sctp_nets *);

int sctp_timer_stop(int, struct sctp_inpcb *, struct sctp_tcb *,
	struct sctp_nets *);

u_int32_t sctp_calculate_sum(struct mbuf *, int32_t *, u_int32_t);

void sctp_mtu_size_reset(struct sctp_inpcb *, struct sctp_association *,
	u_long);

int find_next_best_mtu(int);

u_int32_t sctp_calculate_rto(struct sctp_tcb *, struct sctp_association *,
	struct sctp_nets *, struct timeval *);

u_int32_t sctp_calculate_len(struct mbuf *);

void *sctp_m_getptr(struct mbuf *, int, int, u_int8_t *);

struct sctp_paramhdr *sctp_get_next_param(struct mbuf *, int,
	struct sctp_paramhdr *, int);

int sctp_add_pad_tombuf(struct mbuf *, int);

int sctp_pad_lastmbuf(struct mbuf *, int);

void sctp_ulp_notify(u_int32_t, struct sctp_tcb *, u_int32_t, void *);

void sctp_report_all_outbound(struct sctp_tcb *);

int sctp_expand_mapping_array(struct sctp_association *);

void sctp_abort_notification(struct sctp_tcb *, int);

/* We abort responding to an IP packet for some reason */
void sctp_abort_association(struct sctp_inpcb *, struct sctp_tcb *,
    struct mbuf *, int, struct sctphdr *, struct mbuf *);

/* We choose to abort via user input */
void sctp_abort_an_association(struct sctp_inpcb *, struct sctp_tcb *, int,
	struct mbuf *);

void sctp_handle_ootb(struct mbuf *, int, int, struct sctphdr *,
    struct sctp_inpcb *, struct mbuf *);

int sctp_is_there_an_abort_here(struct mbuf *, int, int *);
uint32_t sctp_is_same_scope(struct sockaddr_in6 *, struct sockaddr_in6 *);
const struct sockaddr_in6 *sctp_recover_scope(const struct sockaddr_in6 *,
	struct sockaddr_in6 *);

int sctp_cmpaddr(const struct sockaddr *, const struct sockaddr *);

void sctp_print_address(const struct sockaddr *);
void sctp_print_address_pkt(struct ip *, struct sctphdr *);

int sbappendaddr_nocheck(struct sockbuf *, const struct sockaddr *,
	struct mbuf *, struct mbuf *, u_int32_t, struct sctp_inpcb *);


int sctp_release_pr_sctp_chunk(struct sctp_tcb *, struct sctp_tmit_chunk *,
	int, struct sctpchunk_listhead *);

struct mbuf *sctp_generate_invmanparam(int);

/*
 * this is an evil layer violation that I think is a hack.. but I stand
 * alone on the tsvwg in this thought... everyone else considers it part
 * of the sockets layer (along with all of the peeloff code :<)
 */
u_int32_t sctp_get_first_vtag_from_sb(struct socket *);


void sctp_grub_through_socket_buffer(struct sctp_inpcb *, struct socket *,
				     struct socket *, struct sctp_tcb *);

void sctp_free_bufspace(struct sctp_tcb *, struct sctp_association *,
	struct sctp_tmit_chunk *);

#ifdef SCTP_STAT_LOGGING
void sctp_log_strm_del_alt(u_int32_t, u_int16_t, int);

void sctp_log_strm_del(struct sctp_tmit_chunk *, struct sctp_tmit_chunk *, int);
void sctp_log_cwnd(struct sctp_nets *, int, uint8_t);
void sctp_log_maxburst(struct sctp_nets *, int, int, uint8_t);
void sctp_log_block(uint8_t, struct socket *, struct sctp_association *);
void sctp_log_rwnd(uint8_t, u_int32_t, u_int32_t, u_int32_t );
void sctp_log_mbcnt(uint8_t, u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void sctp_log_rwnd_set(uint8_t, u_int32_t, u_int32_t, u_int32_t, u_int32_t);
int sctp_fill_stat_log(struct mbuf *);
void sctp_log_fr(uint32_t, uint32_t, uint32_t, int);
void sctp_log_map(uint32_t, uint32_t, uint32_t, int);

void sctp_clr_stat_log(void);

#endif

#ifdef SCTP_AUDITING_ENABLED
void sctp_auditing(int, struct sctp_inpcb *, struct sctp_tcb *,
	struct sctp_nets *);
void sctp_audit_log(u_int8_t, u_int8_t);

#endif

#ifdef SCTP_BASE_FREEBSD
/* Note: these are in <sys/time.h>, but not in kernel space */
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)						\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	    ((tvp)->tv_usec cmp (uvp)->tv_usec) :			\
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define	timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (/* CONSTCOND */ 0)
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (/* CONSTCOND */ 0)
#endif /* SCTP_BASE_FREEBSD */

#endif /* _KERNEL */
#endif
