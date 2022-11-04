/*	$NetBSD: tcp_syncache.c,v 1.6 2022/11/04 09:01:53 ozaki-r Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

/*
 *      @(#)COPYRIGHT   1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 *      This product includes software developed at the Information
 *      Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

/*-
 * Copyright (c) 1997, 1998, 1999, 2001, 2005, 2006,
 * 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Kevin M. Lahey of the Numerical Aerospace Simulation
 * Facility, NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rui Paulo.
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

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 */

/*
 *	TODO list for SYN cache stuff:
 *
 *	Find room for a "state" field, which is needed to keep a
 *	compressed state for TIME_WAIT TCBs.  It's been noted already
 *	that this is fairly important for very high-volume web and
 *	mail servers, which use a large number of short-lived
 *	connections.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcp_syncache.c,v 1.6 2022/11/04 09:01:53 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/pool.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/lwp.h> /* for lwp0 */
#include <sys/cprng.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_private.h>
#include <netinet/tcp_syncache.h>

#ifdef TCP_SIGNATURE
#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#endif	/* IPSEC*/
#endif

static void	syn_cache_timer(void *);
static struct syn_cache *
		syn_cache_lookup(const struct sockaddr *, const struct sockaddr *,
		struct syn_cache_head **);
static int	syn_cache_respond(struct syn_cache *);

/* syn hash parameters */
#define	TCP_SYN_HASH_SIZE	293
#define	TCP_SYN_BUCKET_SIZE	35
static int	tcp_syn_cache_size = TCP_SYN_HASH_SIZE;
int		tcp_syn_cache_limit = TCP_SYN_HASH_SIZE*TCP_SYN_BUCKET_SIZE;
int		tcp_syn_bucket_limit = 3*TCP_SYN_BUCKET_SIZE;
static struct	syn_cache_head tcp_syn_cache[TCP_SYN_HASH_SIZE];

/*
 * TCP compressed state engine.  Currently used to hold compressed
 * state for SYN_RECEIVED.
 */

u_long	syn_cache_count;
static u_int32_t syn_hash1, syn_hash2;

#define SYN_HASH(sa, sp, dp) \
	((((sa)->s_addr^syn_hash1)*(((((u_int32_t)(dp))<<16) + \
				     ((u_int32_t)(sp)))^syn_hash2)))
#ifndef INET6
#define	SYN_HASHALL(hash, src, dst) \
do {									\
	hash = SYN_HASH(&((const struct sockaddr_in *)(src))->sin_addr,	\
		((const struct sockaddr_in *)(src))->sin_port,		\
		((const struct sockaddr_in *)(dst))->sin_port);		\
} while (/*CONSTCOND*/ 0)
#else
#define SYN_HASH6(sa, sp, dp) \
	((((sa)->s6_addr32[0] ^ (sa)->s6_addr32[3] ^ syn_hash1) * \
	  (((((u_int32_t)(dp))<<16) + ((u_int32_t)(sp)))^syn_hash2)) \
	 & 0x7fffffff)

#define SYN_HASHALL(hash, src, dst) \
do {									\
	switch ((src)->sa_family) {					\
	case AF_INET:							\
		hash = SYN_HASH(&((const struct sockaddr_in *)(src))->sin_addr, \
			((const struct sockaddr_in *)(src))->sin_port,	\
			((const struct sockaddr_in *)(dst))->sin_port);	\
		break;							\
	case AF_INET6:							\
		hash = SYN_HASH6(&((const struct sockaddr_in6 *)(src))->sin6_addr, \
			((const struct sockaddr_in6 *)(src))->sin6_port,	\
			((const struct sockaddr_in6 *)(dst))->sin6_port);	\
		break;							\
	default:							\
		hash = 0;						\
	}								\
} while (/*CONSTCOND*/0)
#endif /* INET6 */

static struct pool syn_cache_pool;

/*
 * We don't estimate RTT with SYNs, so each packet starts with the default
 * RTT and each timer step has a fixed timeout value.
 */
static inline void
syn_cache_timer_arm(struct syn_cache *sc)
{

	TCPT_RANGESET(sc->sc_rxtcur,
	    TCPTV_SRTTDFLT * tcp_backoff[sc->sc_rxtshift], TCPTV_MIN,
	    TCPTV_REXMTMAX);
	callout_reset(&sc->sc_timer,
	    sc->sc_rxtcur * (hz / PR_SLOWHZ), syn_cache_timer, sc);
}

#define	SYN_CACHE_TIMESTAMP(sc)	(tcp_now - (sc)->sc_timebase)

static inline void
syn_cache_rm(struct syn_cache *sc)
{
	TAILQ_REMOVE(&tcp_syn_cache[sc->sc_bucketidx].sch_bucket,
	    sc, sc_bucketq);
	sc->sc_tp = NULL;
	LIST_REMOVE(sc, sc_tpq);
	tcp_syn_cache[sc->sc_bucketidx].sch_length--;
	callout_stop(&sc->sc_timer);
	syn_cache_count--;
}

static inline void
syn_cache_put(struct syn_cache *sc)
{
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	rtcache_free(&sc->sc_route);
	sc->sc_flags |= SCF_DEAD;
	if (!callout_invoking(&sc->sc_timer))
		callout_schedule(&(sc)->sc_timer, 1);
}

void
syn_cache_init(void)
{
	int i;

	pool_init(&syn_cache_pool, sizeof(struct syn_cache), 0, 0, 0,
	    "synpl", NULL, IPL_SOFTNET);

	/* Initialize the hash buckets. */
	for (i = 0; i < tcp_syn_cache_size; i++)
		TAILQ_INIT(&tcp_syn_cache[i].sch_bucket);
}

void
syn_cache_insert(struct syn_cache *sc, struct tcpcb *tp)
{
	struct syn_cache_head *scp;
	struct syn_cache *sc2;
	int s;

	/*
	 * If there are no entries in the hash table, reinitialize
	 * the hash secrets.
	 */
	if (syn_cache_count == 0) {
		syn_hash1 = cprng_fast32();
		syn_hash2 = cprng_fast32();
	}

	SYN_HASHALL(sc->sc_hash, &sc->sc_src.sa, &sc->sc_dst.sa);
	sc->sc_bucketidx = sc->sc_hash % tcp_syn_cache_size;
	scp = &tcp_syn_cache[sc->sc_bucketidx];

	/*
	 * Make sure that we don't overflow the per-bucket
	 * limit or the total cache size limit.
	 */
	s = splsoftnet();
	if (scp->sch_length >= tcp_syn_bucket_limit) {
		TCP_STATINC(TCP_STAT_SC_BUCKETOVERFLOW);
		/*
		 * The bucket is full.  Toss the oldest element in the
		 * bucket.  This will be the first entry in the bucket.
		 */
		sc2 = TAILQ_FIRST(&scp->sch_bucket);
#ifdef DIAGNOSTIC
		/*
		 * This should never happen; we should always find an
		 * entry in our bucket.
		 */
		if (sc2 == NULL)
			panic("syn_cache_insert: bucketoverflow: impossible");
#endif
		syn_cache_rm(sc2);
		syn_cache_put(sc2);	/* calls pool_put but see spl above */
	} else if (syn_cache_count >= tcp_syn_cache_limit) {
		struct syn_cache_head *scp2, *sce;

		TCP_STATINC(TCP_STAT_SC_OVERFLOWED);
		/*
		 * The cache is full.  Toss the oldest entry in the
		 * first non-empty bucket we can find.
		 *
		 * XXX We would really like to toss the oldest
		 * entry in the cache, but we hope that this
		 * condition doesn't happen very often.
		 */
		scp2 = scp;
		if (TAILQ_EMPTY(&scp2->sch_bucket)) {
			sce = &tcp_syn_cache[tcp_syn_cache_size];
			for (++scp2; scp2 != scp; scp2++) {
				if (scp2 >= sce)
					scp2 = &tcp_syn_cache[0];
				if (! TAILQ_EMPTY(&scp2->sch_bucket))
					break;
			}
#ifdef DIAGNOSTIC
			/*
			 * This should never happen; we should always find a
			 * non-empty bucket.
			 */
			if (scp2 == scp)
				panic("syn_cache_insert: cacheoverflow: "
				    "impossible");
#endif
		}
		sc2 = TAILQ_FIRST(&scp2->sch_bucket);
		syn_cache_rm(sc2);
		syn_cache_put(sc2);	/* calls pool_put but see spl above */
	}

	/*
	 * Initialize the entry's timer.
	 */
	sc->sc_rxttot = 0;
	sc->sc_rxtshift = 0;
	syn_cache_timer_arm(sc);

	/* Link it from tcpcb entry */
	LIST_INSERT_HEAD(&tp->t_sc, sc, sc_tpq);

	/* Put it into the bucket. */
	TAILQ_INSERT_TAIL(&scp->sch_bucket, sc, sc_bucketq);
	scp->sch_length++;
	syn_cache_count++;

	TCP_STATINC(TCP_STAT_SC_ADDED);
	splx(s);
}

/*
 * Walk the timer queues, looking for SYN,ACKs that need to be retransmitted.
 * If we have retransmitted an entry the maximum number of times, expire
 * that entry.
 */
static void
syn_cache_timer(void *arg)
{
	struct syn_cache *sc = arg;

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);

	callout_ack(&sc->sc_timer);

	if (__predict_false(sc->sc_flags & SCF_DEAD)) {
		TCP_STATINC(TCP_STAT_SC_DELAYED_FREE);
		goto free;
	}

	if (__predict_false(sc->sc_rxtshift == TCP_MAXRXTSHIFT)) {
		/* Drop it -- too many retransmissions. */
		goto dropit;
	}

	/*
	 * Compute the total amount of time this entry has
	 * been on a queue.  If this entry has been on longer
	 * than the keep alive timer would allow, expire it.
	 */
	sc->sc_rxttot += sc->sc_rxtcur;
	if (sc->sc_rxttot >= MIN(tcp_keepinit, TCP_TIMER_MAXTICKS))
		goto dropit;

	TCP_STATINC(TCP_STAT_SC_RETRANSMITTED);
	(void)syn_cache_respond(sc);

	/* Advance the timer back-off. */
	sc->sc_rxtshift++;
	syn_cache_timer_arm(sc);

	goto out;

 dropit:
	TCP_STATINC(TCP_STAT_SC_TIMED_OUT);
	syn_cache_rm(sc);
	if (sc->sc_ipopts)
		(void) m_free(sc->sc_ipopts);
	rtcache_free(&sc->sc_route);

 free:
	callout_destroy(&sc->sc_timer);
	pool_put(&syn_cache_pool, sc);

 out:
	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);
}

/*
 * Remove syn cache created by the specified tcb entry,
 * because this does not make sense to keep them
 * (if there's no tcb entry, syn cache entry will never be used)
 */
void
syn_cache_cleanup(struct tcpcb *tp)
{
	struct syn_cache *sc, *nsc;
	int s;

	s = splsoftnet();

	for (sc = LIST_FIRST(&tp->t_sc); sc != NULL; sc = nsc) {
		nsc = LIST_NEXT(sc, sc_tpq);

#ifdef DIAGNOSTIC
		if (sc->sc_tp != tp)
			panic("invalid sc_tp in syn_cache_cleanup");
#endif
		syn_cache_rm(sc);
		syn_cache_put(sc);	/* calls pool_put but see spl above */
	}
	/* just for safety */
	LIST_INIT(&tp->t_sc);

	splx(s);
}

/*
 * Find an entry in the syn cache.
 */
static struct syn_cache *
syn_cache_lookup(const struct sockaddr *src, const struct sockaddr *dst,
    struct syn_cache_head **headp)
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	u_int32_t hash;
	int s;

	SYN_HASHALL(hash, src, dst);

	scp = &tcp_syn_cache[hash % tcp_syn_cache_size];
	*headp = scp;
	s = splsoftnet();
	for (sc = TAILQ_FIRST(&scp->sch_bucket); sc != NULL;
	     sc = TAILQ_NEXT(sc, sc_bucketq)) {
		if (sc->sc_hash != hash)
			continue;
		if (!memcmp(&sc->sc_src, src, src->sa_len) &&
		    !memcmp(&sc->sc_dst, dst, dst->sa_len)) {
			splx(s);
			return (sc);
		}
	}
	splx(s);
	return (NULL);
}

/*
 * This function gets called when we receive an ACK for a socket in the
 * LISTEN state. We look up the connection in the syn cache, and if it's
 * there, we pull it out of the cache and turn it into a full-blown
 * connection in the SYN-RECEIVED state.
 *
 * The return values may not be immediately obvious, and their effects
 * can be subtle, so here they are:
 *
 *	NULL	SYN was not found in cache; caller should drop the
 *		packet and send an RST.
 *
 *	-1	We were unable to create the new connection, and are
 *		aborting it.  An ACK,RST is being sent to the peer
 *		(unless we got screwey sequence numbers; see below),
 *		because the 3-way handshake has been completed.  Caller
 *		should not free the mbuf, since we may be using it.  If
 *		we are not, we will free it.
 *
 *	Otherwise, the return value is a pointer to the new socket
 *	associated with the connection.
 */
struct socket *
syn_cache_get(struct sockaddr *src, struct sockaddr *dst,
    struct tcphdr *th, struct socket *so, struct mbuf *m)
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct inpcb *inp = NULL;
	struct tcpcb *tp;
	int s;
	struct socket *oso;

	s = splsoftnet();
	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
		splx(s);
		return NULL;
	}

	/*
	 * Verify the sequence and ack numbers.  Try getting the correct
	 * response again.
	 */
	if ((th->th_ack != sc->sc_iss + 1) ||
	    SEQ_LEQ(th->th_seq, sc->sc_irs) ||
	    SEQ_GT(th->th_seq, sc->sc_irs + 1 + sc->sc_win)) {
		m_freem(m);
		(void)syn_cache_respond(sc);
		splx(s);
		return ((struct socket *)(-1));
	}

	/* Remove this cache entry */
	syn_cache_rm(sc);
	splx(s);

	/*
	 * Ok, create the full blown connection, and set things up
	 * as they would have been set up if we had created the
	 * connection when the SYN arrived.  If we can't create
	 * the connection, abort it.
	 */
	/*
	 * inp still has the OLD in_pcb stuff, set the
	 * v6-related flags on the new guy, too.   This is
	 * done particularly for the case where an AF_INET6
	 * socket is bound only to a port, and a v4 connection
	 * comes in on that port.
	 * we also copy the flowinfo from the original pcb
	 * to the new one.
	 */
	oso = so;
	so = sonewconn(so, true);
	if (so == NULL)
		goto resetandabort;

	inp = sotoinpcb(so);

	switch (src->sa_family) {
	case AF_INET:
		if (inp->inp_af == AF_INET) {
			in4p_laddr(inp) = ((struct sockaddr_in *)dst)->sin_addr;
			inp->inp_lport = ((struct sockaddr_in *)dst)->sin_port;
			inp->inp_options = ip_srcroute(m);
			inpcb_set_state(inp, INP_BOUND);
			if (inp->inp_options == NULL) {
				inp->inp_options = sc->sc_ipopts;
				sc->sc_ipopts = NULL;
			}
		}
#ifdef INET6
		else if (inp->inp_af == AF_INET6) {
			/* IPv4 packet to AF_INET6 socket */
			memset(&in6p_laddr(inp), 0, sizeof(in6p_laddr(inp)));
			in6p_laddr(inp).s6_addr16[5] = htons(0xffff);
			bcopy(&((struct sockaddr_in *)dst)->sin_addr,
				&in6p_laddr(inp).s6_addr32[3],
				sizeof(((struct sockaddr_in *)dst)->sin_addr));
			inp->inp_lport = ((struct sockaddr_in *)dst)->sin_port;
			intotcpcb(inp)->t_family = AF_INET;
			if (sotoinpcb(oso)->inp_flags & IN6P_IPV6_V6ONLY)
				inp->inp_flags |= IN6P_IPV6_V6ONLY;
			else
				inp->inp_flags &= ~IN6P_IPV6_V6ONLY;
			inpcb_set_state(inp, INP_BOUND);
		}
#endif
		break;
#ifdef INET6
	case AF_INET6:
		if (inp->inp_af == AF_INET6) {
			in6p_laddr(inp) = ((struct sockaddr_in6 *)dst)->sin6_addr;
			inp->inp_lport = ((struct sockaddr_in6 *)dst)->sin6_port;
			inpcb_set_state(inp, INP_BOUND);
		}
		break;
#endif
	}

#ifdef INET6
	if (inp && intotcpcb(inp)->t_family == AF_INET6 && sotoinpcb(oso)) {
		struct inpcb *oinp = sotoinpcb(oso);
		/* inherit socket options from the listening socket */
		inp->inp_flags |= (oinp->inp_flags & IN6P_CONTROLOPTS);
		if (inp->inp_flags & IN6P_CONTROLOPTS) {
			m_freem(inp->inp_options);
			inp->inp_options = NULL;
		}
		ip6_savecontrol(inp, &inp->inp_options,
		    mtod(m, struct ip6_hdr *), m);
	}
#endif

	/*
	 * Give the new socket our cached route reference.
	 */
	rtcache_copy(&inp->inp_route, &sc->sc_route);
	rtcache_free(&sc->sc_route);

	if (inp->inp_af == AF_INET) {
		struct sockaddr_in sin;
		memcpy(&sin, src, src->sa_len);
		if (inpcb_connect(inp, &sin, &lwp0)) {
			goto resetandabort;
		}
	}
#ifdef INET6
	else if (inp->inp_af == AF_INET6) {
		struct sockaddr_in6 sin6;
		memcpy(&sin6, src, src->sa_len);
		if (src->sa_family == AF_INET) {
			/* IPv4 packet to AF_INET6 socket */
			in6_sin_2_v4mapsin6((struct sockaddr_in *)src, &sin6);
		}
		if (in6pcb_connect(inp, &sin6, NULL)) {
			goto resetandabort;
		}
	}
#endif
	else {
		goto resetandabort;
	}

	tp = intotcpcb(inp);

	tp->t_flags = sototcpcb(oso)->t_flags & TF_NODELAY;
	if (sc->sc_request_r_scale != 15) {
		tp->requested_s_scale = sc->sc_requested_s_scale;
		tp->request_r_scale = sc->sc_request_r_scale;
		tp->snd_scale = sc->sc_requested_s_scale;
		tp->rcv_scale = sc->sc_request_r_scale;
		tp->t_flags |= TF_REQ_SCALE|TF_RCVD_SCALE;
	}
	if (sc->sc_flags & SCF_TIMESTAMP)
		tp->t_flags |= TF_REQ_TSTMP|TF_RCVD_TSTMP;
	tp->ts_timebase = sc->sc_timebase;

	tp->t_template = tcp_template(tp);
	if (tp->t_template == 0) {
		tp = tcp_drop(tp, ENOBUFS);	/* destroys socket */
		so = NULL;
		m_freem(m);
		goto abort;
	}

	tp->iss = sc->sc_iss;
	tp->irs = sc->sc_irs;
	tcp_sendseqinit(tp);
	tcp_rcvseqinit(tp);
	tp->t_state = TCPS_SYN_RECEIVED;
	TCP_TIMER_ARM(tp, TCPT_KEEP, tp->t_keepinit);
	TCP_STATINC(TCP_STAT_ACCEPTS);

	if ((sc->sc_flags & SCF_SACK_PERMIT) && tcp_do_sack)
		tp->t_flags |= TF_WILL_SACK;

	if ((sc->sc_flags & SCF_ECN_PERMIT) && tcp_do_ecn)
		tp->t_flags |= TF_ECN_PERMIT;

#ifdef TCP_SIGNATURE
	if (sc->sc_flags & SCF_SIGNATURE)
		tp->t_flags |= TF_SIGNATURE;
#endif

	/* Initialize tp->t_ourmss before we deal with the peer's! */
	tp->t_ourmss = sc->sc_ourmaxseg;
	tcp_mss_from_peer(tp, sc->sc_peermaxseg);

	/*
	 * Initialize the initial congestion window.  If we
	 * had to retransmit the SYN,ACK, we must initialize cwnd
	 * to 1 segment (i.e. the Loss Window).
	 */
	if (sc->sc_rxtshift)
		tp->snd_cwnd = tp->t_peermss;
	else {
		int ss = tcp_init_win;
		if (inp->inp_af == AF_INET && in_localaddr(in4p_faddr(inp)))
			ss = tcp_init_win_local;
#ifdef INET6
		else if (inp->inp_af == AF_INET6 && in6_localaddr(&in6p_faddr(inp)))
			ss = tcp_init_win_local;
#endif
		tp->snd_cwnd = TCP_INITIAL_WINDOW(ss, tp->t_peermss);
	}

	tcp_rmx_rtt(tp);
	tp->snd_wl1 = sc->sc_irs;
	tp->rcv_up = sc->sc_irs + 1;

	/*
	 * This is what would have happened in tcp_output() when
	 * the SYN,ACK was sent.
	 */
	tp->snd_up = tp->snd_una;
	tp->snd_max = tp->snd_nxt = tp->iss+1;
	TCP_TIMER_ARM(tp, TCPT_REXMT, tp->t_rxtcur);
	if (sc->sc_win > 0 && SEQ_GT(tp->rcv_nxt + sc->sc_win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + sc->sc_win;
	tp->last_ack_sent = tp->rcv_nxt;
	tp->t_partialacks = -1;
	tp->t_dupacks = 0;

	TCP_STATINC(TCP_STAT_SC_COMPLETED);
	s = splsoftnet();
	syn_cache_put(sc);
	splx(s);
	return so;

resetandabort:
	(void)tcp_respond(NULL, m, m, th, (tcp_seq)0, th->th_ack, TH_RST);
abort:
	if (so != NULL) {
		(void) soqremque(so, 1);
		(void) soabort(so);
		mutex_enter(softnet_lock);
	}
	s = splsoftnet();
	syn_cache_put(sc);
	splx(s);
	TCP_STATINC(TCP_STAT_SC_ABORTED);
	return ((struct socket *)(-1));
}

/*
 * This function is called when we get a RST for a
 * non-existent connection, so that we can see if the
 * connection is in the syn cache.  If it is, zap it.
 */

void
syn_cache_reset(struct sockaddr *src, struct sockaddr *dst, struct tcphdr *th)
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	int s = splsoftnet();

	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
		splx(s);
		return;
	}
	if (SEQ_LT(th->th_seq, sc->sc_irs) ||
	    SEQ_GT(th->th_seq, sc->sc_irs+1)) {
		splx(s);
		return;
	}
	syn_cache_rm(sc);
	TCP_STATINC(TCP_STAT_SC_RESET);
	syn_cache_put(sc);	/* calls pool_put but see spl above */
	splx(s);
}

void
syn_cache_unreach(const struct sockaddr *src, const struct sockaddr *dst,
    struct tcphdr *th)
{
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	int s;

	s = splsoftnet();
	if ((sc = syn_cache_lookup(src, dst, &scp)) == NULL) {
		splx(s);
		return;
	}
	/* If the sequence number != sc_iss, then it's a bogus ICMP msg */
	if (ntohl(th->th_seq) != sc->sc_iss) {
		splx(s);
		return;
	}

	/*
	 * If we've retransmitted 3 times and this is our second error,
	 * we remove the entry.  Otherwise, we allow it to continue on.
	 * This prevents us from incorrectly nuking an entry during a
	 * spurious network outage.
	 *
	 * See tcp_notify().
	 */
	if ((sc->sc_flags & SCF_UNREACH) == 0 || sc->sc_rxtshift < 3) {
		sc->sc_flags |= SCF_UNREACH;
		splx(s);
		return;
	}

	syn_cache_rm(sc);
	TCP_STATINC(TCP_STAT_SC_UNREACH);
	syn_cache_put(sc);	/* calls pool_put but see spl above */
	splx(s);
}

/*
 * Given a LISTEN socket and an inbound SYN request, add this to the syn
 * cache, and send back a segment:
 *	<SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
 * to the source.
 *
 * IMPORTANT NOTE: We do _NOT_ ACK data that might accompany the SYN.
 * Doing so would require that we hold onto the data and deliver it
 * to the application.  However, if we are the target of a SYN-flood
 * DoS attack, an attacker could send data which would eventually
 * consume all available buffer space if it were ACKed.  By not ACKing
 * the data, we avoid this DoS scenario.
 */
int
syn_cache_add(struct sockaddr *src, struct sockaddr *dst, struct tcphdr *th,
    unsigned int toff, struct socket *so, struct mbuf *m, u_char *optp,
    int optlen, struct tcp_opt_info *oi)
{
	struct tcpcb tb, *tp;
	long win;
	struct syn_cache *sc;
	struct syn_cache_head *scp;
	struct mbuf *ipopts;
	int s;

	tp = sototcpcb(so);

	/*
	 * Initialize some local state.
	 */
	win = sbspace(&so->so_rcv);
	if (win > TCP_MAXWIN)
		win = TCP_MAXWIN;

#ifdef TCP_SIGNATURE
	if (optp || (tp->t_flags & TF_SIGNATURE))
#else
	if (optp)
#endif
	{
		tb.t_flags = tcp_do_rfc1323 ? (TF_REQ_SCALE|TF_REQ_TSTMP) : 0;
#ifdef TCP_SIGNATURE
		tb.t_flags |= (tp->t_flags & TF_SIGNATURE);
#endif
		tb.t_state = TCPS_LISTEN;
		if (tcp_dooptions(&tb, optp, optlen, th, m, toff, oi) < 0)
			return 0;
	} else
		tb.t_flags = 0;

	switch (src->sa_family) {
	case AF_INET:
		/* Remember the IP options, if any. */
		ipopts = ip_srcroute(m);
		break;
	default:
		ipopts = NULL;
	}

	/*
	 * See if we already have an entry for this connection.
	 * If we do, resend the SYN,ACK.  We do not count this
	 * as a retransmission (XXX though maybe we should).
	 */
	if ((sc = syn_cache_lookup(src, dst, &scp)) != NULL) {
		TCP_STATINC(TCP_STAT_SC_DUPESYN);
		if (ipopts) {
			/*
			 * If we were remembering a previous source route,
			 * forget it and use the new one we've been given.
			 */
			if (sc->sc_ipopts)
				(void)m_free(sc->sc_ipopts);
			sc->sc_ipopts = ipopts;
		}
		sc->sc_timestamp = tb.ts_recent;
		m_freem(m);
		if (syn_cache_respond(sc) == 0) {
			uint64_t *tcps = TCP_STAT_GETREF();
			tcps[TCP_STAT_SNDACKS]++;
			tcps[TCP_STAT_SNDTOTAL]++;
			TCP_STAT_PUTREF();
		}
		return 1;
	}

	s = splsoftnet();
	sc = pool_get(&syn_cache_pool, PR_NOWAIT);
	splx(s);
	if (sc == NULL) {
		if (ipopts)
			(void)m_free(ipopts);
		return 0;
	}

	/*
	 * Fill in the cache, and put the necessary IP and TCP
	 * options into the reply.
	 */
	memset(sc, 0, sizeof(struct syn_cache));
	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
	memcpy(&sc->sc_src, src, src->sa_len);
	memcpy(&sc->sc_dst, dst, dst->sa_len);
	sc->sc_flags = 0;
	sc->sc_ipopts = ipopts;
	sc->sc_irs = th->th_seq;
	switch (src->sa_family) {
	case AF_INET:
	    {
		struct sockaddr_in *srcin = (void *)src;
		struct sockaddr_in *dstin = (void *)dst;

		sc->sc_iss = tcp_new_iss1(&dstin->sin_addr,
		    &srcin->sin_addr, dstin->sin_port,
		    srcin->sin_port, sizeof(dstin->sin_addr));
		break;
	    }
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 *srcin6 = (void *)src;
		struct sockaddr_in6 *dstin6 = (void *)dst;

		sc->sc_iss = tcp_new_iss1(&dstin6->sin6_addr,
		    &srcin6->sin6_addr, dstin6->sin6_port,
		    srcin6->sin6_port, sizeof(dstin6->sin6_addr));
		break;
	    }
#endif
	}
	sc->sc_peermaxseg = oi->maxseg;
	sc->sc_ourmaxseg = tcp_mss_to_advertise(m->m_flags & M_PKTHDR ?
	    m_get_rcvif_NOMPSAFE(m) : NULL, sc->sc_src.sa.sa_family);
	sc->sc_win = win;
	sc->sc_timebase = tcp_now - 1;	/* see tcp_newtcpcb() */
	sc->sc_timestamp = tb.ts_recent;
	if ((tb.t_flags & (TF_REQ_TSTMP|TF_RCVD_TSTMP)) ==
	    (TF_REQ_TSTMP|TF_RCVD_TSTMP))
		sc->sc_flags |= SCF_TIMESTAMP;
	if ((tb.t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) ==
	    (TF_RCVD_SCALE|TF_REQ_SCALE)) {
		sc->sc_requested_s_scale = tb.requested_s_scale;
		sc->sc_request_r_scale = 0;
		/*
		 * Pick the smallest possible scaling factor that
		 * will still allow us to scale up to sb_max.
		 *
		 * We do this because there are broken firewalls that
		 * will corrupt the window scale option, leading to
		 * the other endpoint believing that our advertised
		 * window is unscaled.  At scale factors larger than
		 * 5 the unscaled window will drop below 1500 bytes,
		 * leading to serious problems when traversing these
		 * broken firewalls.
		 *
		 * With the default sbmax of 256K, a scale factor
		 * of 3 will be chosen by this algorithm.  Those who
		 * choose a larger sbmax should watch out
		 * for the compatibility problems mentioned above.
		 *
		 * RFC1323: The Window field in a SYN (i.e., a <SYN>
		 * or <SYN,ACK>) segment itself is never scaled.
		 */
		while (sc->sc_request_r_scale < TCP_MAX_WINSHIFT &&
		    (TCP_MAXWIN << sc->sc_request_r_scale) < sb_max)
			sc->sc_request_r_scale++;
	} else {
		sc->sc_requested_s_scale = 15;
		sc->sc_request_r_scale = 15;
	}
	if ((tb.t_flags & TF_SACK_PERMIT) && tcp_do_sack)
		sc->sc_flags |= SCF_SACK_PERMIT;

	/*
	 * ECN setup packet received.
	 */
	if ((th->th_flags & (TH_ECE|TH_CWR)) && tcp_do_ecn)
		sc->sc_flags |= SCF_ECN_PERMIT;

#ifdef TCP_SIGNATURE
	if (tb.t_flags & TF_SIGNATURE)
		sc->sc_flags |= SCF_SIGNATURE;
#endif
	sc->sc_tp = tp;
	m_freem(m);
	if (syn_cache_respond(sc) == 0) {
		uint64_t *tcps = TCP_STAT_GETREF();
		tcps[TCP_STAT_SNDACKS]++;
		tcps[TCP_STAT_SNDTOTAL]++;
		TCP_STAT_PUTREF();
		syn_cache_insert(sc, tp);
	} else {
		s = splsoftnet();
		/*
		 * syn_cache_put() will try to schedule the timer, so
		 * we need to initialize it
		 */
		syn_cache_timer_arm(sc);
		syn_cache_put(sc);
		splx(s);
		TCP_STATINC(TCP_STAT_SC_DROPPED);
	}
	return 1;
}

/*
 * syn_cache_respond: (re)send SYN+ACK.
 *
 * Returns 0 on success.
 */

static int
syn_cache_respond(struct syn_cache *sc)
{
#ifdef INET6
	struct rtentry *rt = NULL;
#endif
	struct route *ro;
	u_int8_t *optp;
	int optlen, error;
	u_int16_t tlen;
	struct ip *ip = NULL;
#ifdef INET6
	struct ip6_hdr *ip6 = NULL;
#endif
	struct tcpcb *tp;
	struct tcphdr *th;
	struct mbuf *m;
	u_int hlen;
#ifdef TCP_SIGNATURE
	struct secasvar *sav = NULL;
	u_int8_t *sigp = NULL;
#endif

	ro = &sc->sc_route;
	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		hlen = sizeof(struct ip);
		break;
#ifdef INET6
	case AF_INET6:
		hlen = sizeof(struct ip6_hdr);
		break;
#endif
	default:
		return EAFNOSUPPORT;
	}

	/* Worst case scenario, since we don't know the option size yet. */
	tlen = hlen + sizeof(struct tcphdr) + MAX_TCPOPTLEN;
	KASSERT(max_linkhdr + tlen <= MCLBYTES);

	/*
	 * Create the IP+TCP header from scratch.
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && (max_linkhdr + tlen) > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		}
	}
	if (m == NULL)
		return ENOBUFS;
	MCLAIM(m, &tcp_tx_mowner);

	tp = sc->sc_tp;

	/* Fixup the mbuf. */
	m->m_data += max_linkhdr;
	m_reset_rcvif(m);
	memset(mtod(m, void *), 0, tlen);

	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		ip = mtod(m, struct ip *);
		ip->ip_v = 4;
		ip->ip_dst = sc->sc_src.sin.sin_addr;
		ip->ip_src = sc->sc_dst.sin.sin_addr;
		ip->ip_p = IPPROTO_TCP;
		th = (struct tcphdr *)(ip + 1);
		th->th_dport = sc->sc_src.sin.sin_port;
		th->th_sport = sc->sc_dst.sin.sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_vfc = IPV6_VERSION;
		ip6->ip6_dst = sc->sc_src.sin6.sin6_addr;
		ip6->ip6_src = sc->sc_dst.sin6.sin6_addr;
		ip6->ip6_nxt = IPPROTO_TCP;
		/* ip6_plen will be updated in ip6_output() */
		th = (struct tcphdr *)(ip6 + 1);
		th->th_dport = sc->sc_src.sin6.sin6_port;
		th->th_sport = sc->sc_dst.sin6.sin6_port;
		break;
#endif
	default:
		panic("%s: impossible (1)", __func__);
	}

	th->th_seq = htonl(sc->sc_iss);
	th->th_ack = htonl(sc->sc_irs + 1);
	th->th_flags = TH_SYN|TH_ACK;
	th->th_win = htons(sc->sc_win);
	/* th_x2, th_sum, th_urp already 0 from memset */

	/* Tack on the TCP options. */
	optp = (u_int8_t *)(th + 1);
	optlen = 0;
	*optp++ = TCPOPT_MAXSEG;
	*optp++ = TCPOLEN_MAXSEG;
	*optp++ = (sc->sc_ourmaxseg >> 8) & 0xff;
	*optp++ = sc->sc_ourmaxseg & 0xff;
	optlen += TCPOLEN_MAXSEG;

	if (sc->sc_request_r_scale != 15) {
		*((u_int32_t *)optp) = htonl(TCPOPT_NOP << 24 |
		    TCPOPT_WINDOW << 16 | TCPOLEN_WINDOW << 8 |
		    sc->sc_request_r_scale);
		optp += TCPOLEN_WINDOW + TCPOLEN_NOP;
		optlen += TCPOLEN_WINDOW + TCPOLEN_NOP;
	}

	if (sc->sc_flags & SCF_SACK_PERMIT) {
		/* Let the peer know that we will SACK. */
		*optp++ = TCPOPT_SACK_PERMITTED;
		*optp++ = TCPOLEN_SACK_PERMITTED;
		optlen += TCPOLEN_SACK_PERMITTED;
	}

	if (sc->sc_flags & SCF_TIMESTAMP) {
		while (optlen % 4 != 2) {
			optlen += TCPOLEN_NOP;
			*optp++ = TCPOPT_NOP;
		}
		*optp++ = TCPOPT_TIMESTAMP;
		*optp++ = TCPOLEN_TIMESTAMP;
		u_int32_t *lp = (u_int32_t *)(optp);
		/* Form timestamp option as shown in appendix A of RFC 1323. */
		*lp++ = htonl(SYN_CACHE_TIMESTAMP(sc));
		*lp   = htonl(sc->sc_timestamp);
		optp += TCPOLEN_TIMESTAMP - 2;
		optlen += TCPOLEN_TIMESTAMP;
	}

#ifdef TCP_SIGNATURE
	if (sc->sc_flags & SCF_SIGNATURE) {
		sav = tcp_signature_getsav(m);
		if (sav == NULL) {
			m_freem(m);
			return EPERM;
		}

		*optp++ = TCPOPT_SIGNATURE;
		*optp++ = TCPOLEN_SIGNATURE;
		sigp = optp;
		memset(optp, 0, TCP_SIGLEN);
		optp += TCP_SIGLEN;
		optlen += TCPOLEN_SIGNATURE;
	}
#endif

	/*
	 * Terminate and pad TCP options to a 4 byte boundary.
	 *
	 * According to RFC793: "The content of the header beyond the
	 * End-of-Option option must be header padding (i.e., zero)."
	 * And later: "The padding is composed of zeros."
	 */
	if (optlen % 4) {
		optlen += TCPOLEN_EOL;
		*optp++ = TCPOPT_EOL;
	}
	while (optlen % 4) {
		optlen += TCPOLEN_PAD;
		*optp++ = TCPOPT_PAD;
	}

	/* Compute the actual values now that we've added the options. */
	tlen = hlen + sizeof(struct tcphdr) + optlen;
	m->m_len = m->m_pkthdr.len = tlen;
	th->th_off = (sizeof(struct tcphdr) + optlen) >> 2;

#ifdef TCP_SIGNATURE
	if (sav) {
		(void)tcp_signature(m, th, hlen, sav, sigp);
		key_sa_recordxfer(sav, m);
		KEY_SA_UNREF(&sav);
	}
#endif

	/*
	 * Send ECN SYN-ACK setup packet.
	 * Routes can be asymmetric, so, even if we receive a packet
	 * with ECE and CWR set, we must not assume no one will block
	 * the ECE packet we are about to send.
	 */
	if ((sc->sc_flags & SCF_ECN_PERMIT) && tp &&
	    SEQ_GEQ(tp->snd_nxt, tp->snd_max)) {
		th->th_flags |= TH_ECE;
		TCP_STATINC(TCP_STAT_ECN_SHS);

		/*
		 * draft-ietf-tcpm-ecnsyn-00.txt
		 *
		 * "[...] a TCP node MAY respond to an ECN-setup
		 * SYN packet by setting ECT in the responding
		 * ECN-setup SYN/ACK packet, indicating to routers 
		 * that the SYN/ACK packet is ECN-Capable.
		 * This allows a congested router along the path
		 * to mark the packet instead of dropping the
		 * packet as an indication of congestion."
		 *
		 * "[...] There can be a great benefit in setting
		 * an ECN-capable codepoint in SYN/ACK packets [...]
		 * Congestion is  most likely to occur in
		 * the server-to-client direction.  As a result,
		 * setting an ECN-capable codepoint in SYN/ACK
		 * packets can reduce the occurrence of three-second
		 * retransmit timeouts resulting from the drop
		 * of SYN/ACK packets."
		 *
		 * Page 4 and 6, January 2006.
		 */

		switch (sc->sc_src.sa.sa_family) {
		case AF_INET:
			ip->ip_tos |= IPTOS_ECN_ECT0;
			break;
#ifdef INET6
		case AF_INET6:
			ip6->ip6_flow |= htonl(IPTOS_ECN_ECT0 << 20);
			break;
#endif
		}
		TCP_STATINC(TCP_STAT_ECN_ECT);
	}


	/*
	 * Compute the packet's checksum.
	 *
	 * Fill in some straggling IP bits.  Note the stack expects
	 * ip_len to be in host order, for convenience.
	 */
	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		ip->ip_len = htons(tlen - hlen);
		th->th_sum = 0;
		th->th_sum = in4_cksum(m, IPPROTO_TCP, hlen, tlen - hlen);
		ip->ip_len = htons(tlen);
		ip->ip_ttl = ip_defttl;
		/* XXX tos? */
		break;
#ifdef INET6
	case AF_INET6:
		ip6->ip6_plen = htons(tlen - hlen);
		th->th_sum = 0;
		th->th_sum = in6_cksum(m, IPPROTO_TCP, hlen, tlen - hlen);
		ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6->ip6_vfc |= IPV6_VERSION;
		ip6->ip6_plen = htons(tlen - hlen);
		/* ip6_hlim will be initialized afterwards */
		/* XXX flowlabel? */
		break;
#endif
	}

	/* XXX use IPsec policy on listening socket, on SYN ACK */
	tp = sc->sc_tp;

	switch (sc->sc_src.sa.sa_family) {
	case AF_INET:
		error = ip_output(m, sc->sc_ipopts, ro,
		    (ip_mtudisc ? IP_MTUDISC : 0),
		    NULL, tp ? tp->t_inpcb : NULL);
		break;
#ifdef INET6
	case AF_INET6:
		ip6->ip6_hlim = in6pcb_selecthlim(NULL,
		    (rt = rtcache_validate(ro)) != NULL ? rt->rt_ifp : NULL);
		rtcache_unref(rt, ro);

		error = ip6_output(m, NULL /*XXX*/, ro, 0, NULL,
		    tp ? tp->t_inpcb : NULL, NULL);
		break;
#endif
	default:
		panic("%s: impossible (2)", __func__);
	}

	return error;
}
