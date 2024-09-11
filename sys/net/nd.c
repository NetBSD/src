/*	$NetBSD: nd.c,v 1.5.2.1 2024/09/11 16:18:36 martin Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nd.c,v 1.5.2.1 2024/09/11 16:18:36 martin Exp $");

#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/socketvar.h> /* for softnet_lock */

#include <net/if_llatbl.h>
#include <net/nd.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/ip6.h>

static struct nd_domain *nd_domains[AF_MAX];

static int nd_gctimer = (60 * 60 * 24); /* 1 day: garbage collection timer */

static void nd_set_timertick(struct llentry *, time_t);
static struct nd_domain *nd_find_domain(int);

static void
nd_timer(void *arg)
{
	struct llentry *ln = arg;
	struct nd_domain *nd;
	struct ifnet *ifp = NULL;
	struct psref psref;
	struct mbuf *m = NULL;
	bool send_ns = false;
	int16_t missed = ND_LLINFO_NOSTATE;
	union l3addr taddr, *daddrp = NULL;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	LLE_WLOCK(ln);

	if (!(ln->la_flags & LLE_LINKED))
		goto out;
	if (ln->ln_ntick > 0) {
		nd_set_timer(ln, ND_TIMER_TICK);
		goto out;
	}

	nd = nd_find_domain(ln->lle_tbl->llt_af);
	ifp = ln->lle_tbl->llt_ifp;
	KASSERT(ifp != NULL);
	if_acquire(ifp, &psref);

	memcpy(&taddr, &ln->r_l3addr, sizeof(taddr));

	switch (ln->ln_state) {
	case ND_LLINFO_WAITDELETE:
		LLE_REMREF(ln);
		nd->nd_free(ln, 0);
		ln = NULL;
		break;

	case ND_LLINFO_INCOMPLETE:
		send_ns = true;
		if (ln->ln_asked++ < nd->nd_mmaxtries)
			break;

		if (ln->ln_hold) {
			struct mbuf *m0, *mnxt;

			/*
			 * Assuming every packet in ln_hold
			 * has the same IP header.
			 */
			m = ln->ln_hold;
			for (m0 = m->m_nextpkt; m0 != NULL; m0 = mnxt) {
				mnxt = m0->m_nextpkt;
				m0->m_nextpkt = NULL;
				m_freem(m0);
			}

			m->m_nextpkt = NULL;
			ln->ln_hold = NULL;
			ln->la_numheld = 0;
		}

		KASSERTMSG(ln->la_numheld == 0, "la_numheld=%d",
		    ln->la_numheld);

		missed = ND_LLINFO_INCOMPLETE;
		ln->ln_state = ND_LLINFO_WAITDELETE;
		break;

	case ND_LLINFO_REACHABLE:
		if (!ND_IS_LLINFO_PERMANENT(ln)) {
			ln->ln_state = ND_LLINFO_STALE;
			nd_set_timer(ln, ND_TIMER_GC);
		}
		break;

	case ND_LLINFO_PURGE: /* FALLTHROUGH */
	case ND_LLINFO_STALE:
		if (!ND_IS_LLINFO_PERMANENT(ln)) {
			LLE_REMREF(ln);
			nd->nd_free(ln, 1);
			ln = NULL;
		}
		break;

	case ND_LLINFO_DELAY:
		if (nd->nd_nud_enabled(ifp)) {
			ln->ln_asked = 1;
			ln->ln_state = ND_LLINFO_PROBE;
			send_ns = true;
			daddrp = &taddr;
		} else {
			ln->ln_state = ND_LLINFO_STALE;
			nd_set_timer(ln, ND_TIMER_GC);
		}
		break;

	case ND_LLINFO_PROBE:
		send_ns = true;
		if (ln->ln_asked++ < nd->nd_umaxtries) {
			daddrp = &taddr;
		} else {
			ln->ln_state = ND_LLINFO_UNREACHABLE;
			ln->ln_asked = 1;
			missed = ND_LLINFO_PROBE;
			/* nd_missed() consumers can use missed to know if
			 * they need to send ICMP UNREACHABLE or not. */
		}
		break;
	case ND_LLINFO_UNREACHABLE:
		/*
		 * RFC 7048 Section 3 says in the UNREACHABLE state
		 * packets continue to be sent to the link-layer address and
		 * then backoff exponentially.
		 * We adjust this slightly and move to the INCOMPLETE state
		 * after nd_mmaxtries probes and then start backing off.
		 *
		 * This results in simpler code whilst providing a more robust
		 * model which doubles the time to failure over what we did
		 * before. We don't want to be back to the old ARP model where
		 * no unreachability errors are returned because very
		 * few applications would look at unreachability hints provided
		 * such as ND_LLINFO_UNREACHABLE or RTM_MISS.
		 */
		send_ns = true;
		if (ln->ln_asked++ < nd->nd_mmaxtries)
			break;

		missed = ND_LLINFO_UNREACHABLE;
		ln->ln_state = ND_LLINFO_WAITDELETE;
		ln->la_flags &= ~LLE_VALID;
		break;
	}

	if (send_ns) {
		uint8_t lladdr[255], *lladdrp;
		union l3addr src, *psrc;

		if (ln->ln_state == ND_LLINFO_WAITDELETE)
			nd_set_timer(ln, ND_TIMER_RETRANS_BACKOFF);
		else
			nd_set_timer(ln, ND_TIMER_RETRANS);
		if (ln->ln_state > ND_LLINFO_INCOMPLETE &&
		    ln->la_flags & LLE_VALID)
		{
			KASSERT(sizeof(lladdr) >= ifp->if_addrlen);
			memcpy(lladdr, &ln->ll_addr, ifp->if_addrlen);
			lladdrp = lladdr;
		} else
			lladdrp = NULL;
		psrc = nd->nd_holdsrc(ln, &src);
		LLE_FREE_LOCKED(ln);
		ln = NULL;
		nd->nd_output(ifp, daddrp, &taddr, lladdrp, psrc);
	}

out:
	if (ln != NULL)
		LLE_FREE_LOCKED(ln);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();

	if (missed != ND_LLINFO_NOSTATE)
		nd->nd_missed(ifp, &taddr, missed, m);
	if (ifp != NULL)
		if_release(ifp, &psref);
}

static void
nd_set_timertick(struct llentry *ln, time_t xtick)
{

	CTASSERT(sizeof(time_t) > sizeof(int));
	KASSERT(xtick >= 0);

	/*
	 * We have to take care of a reference leak which occurs if
	 * callout_reset overwrites a pending callout schedule.  Unfortunately
	 * we don't have a mean to know the overwrite, so we need to know it
	 * using callout_stop.  We need to call callout_pending first to exclude
	 * the case that the callout has never been scheduled.
	 */
	if (callout_pending(&ln->la_timer)) {
		bool expired;

		expired = callout_stop(&ln->la_timer);
		if (!expired)
			LLE_REMREF(ln);
	}

	ln->ln_expire = time_uptime + xtick / hz;
	LLE_ADDREF(ln);
	if (xtick > INT_MAX) {
		ln->ln_ntick = xtick - INT_MAX;
		xtick = INT_MAX;
	} else {
		ln->ln_ntick = 0;
	}
	callout_reset(&ln->ln_timer_ch, xtick, nd_timer, ln);
}

void
nd_set_timer(struct llentry *ln, int type)
{
	time_t xtick;
	struct ifnet *ifp;
	struct nd_domain *nd;

	LLE_WLOCK_ASSERT(ln);

	ifp = ln->lle_tbl->llt_ifp;
	nd = nd_find_domain(ln->lle_tbl->llt_af);

	switch (type) {
	case ND_TIMER_IMMEDIATE:
		xtick = 0;
		break;
	case ND_TIMER_TICK:
		xtick = ln->ln_ntick;
		break;
	case ND_TIMER_RETRANS:
		xtick = nd->nd_retrans(ifp) * hz / 1000;
		break;
	case ND_TIMER_RETRANS_BACKOFF:
	{
		unsigned int retrans = nd->nd_retrans(ifp);
		unsigned int attempts = ln->ln_asked - nd->nd_mmaxtries;

		xtick = retrans;
		while (attempts-- != 0) {
			xtick *= nd->nd_retransmultiple;
			if (xtick > nd->nd_maxretrans || xtick < retrans) {
				xtick = nd->nd_maxretrans;
				break;
			}
		}
		xtick = xtick * hz / 1000;
		break;
	}
	case ND_TIMER_REACHABLE:
		xtick = nd->nd_reachable(ifp) * hz / 1000;
		break;
	case ND_TIMER_EXPIRE:
		if (ln->ln_expire > time_uptime)
			xtick = (ln->ln_expire - time_uptime) * hz;
		else
			xtick = nd_gctimer * hz;
		break;
	case ND_TIMER_DELAY:
		xtick = nd->nd_delay * hz;
		break;
	case ND_TIMER_GC:
		xtick = nd_gctimer * hz;
		break;
	default:
		panic("%s: invalid timer type\n", __func__);
	}

	nd_set_timertick(ln, xtick);
}

int
nd_resolve(struct llentry *ln, const struct rtentry *rt, struct mbuf *m,
    uint8_t *lldst, size_t dstsize)
{
	struct ifnet *ifp;
	struct nd_domain *nd;
	int error;

	LLE_WLOCK_ASSERT(ln);

	ifp = ln->lle_tbl->llt_ifp;
	nd = nd_find_domain(ln->lle_tbl->llt_af);

	/* We don't have to do link-layer address resolution on a p2p link. */
	if (ifp->if_flags & IFF_POINTOPOINT &&
	    ln->ln_state < ND_LLINFO_REACHABLE)
	{
		ln->ln_state = ND_LLINFO_STALE;
		nd_set_timer(ln, ND_TIMER_GC);
	}

	/*
	 * The first time we send a packet to a neighbor whose entry is
	 * STALE, we have to change the state to DELAY and a sets a timer to
	 * expire in DELAY_FIRST_PROBE_TIME seconds to ensure do
	 * neighbor unreachability detection on expiration.
	 * (RFC 2461 7.3.3)
	 */
	if (ln->ln_state == ND_LLINFO_STALE) {
		ln->ln_asked = 0;
		ln->ln_state = ND_LLINFO_DELAY;
		nd_set_timer(ln, ND_TIMER_DELAY);
	}

	/*
	 * If the neighbor cache entry has a state other than INCOMPLETE
	 * (i.e. its link-layer address is already resolved), just
	 * send the packet.
	 */
	if (ln->ln_state > ND_LLINFO_INCOMPLETE) {
		KASSERT((ln->la_flags & LLE_VALID) != 0);
		memcpy(lldst, &ln->ll_addr, MIN(dstsize, ifp->if_addrlen));
		LLE_WUNLOCK(ln);
		return 0;
	}

	/*
	 * There is a neighbor cache entry, but no ethernet address
	 * response yet.  Append this latest packet to the end of the
	 * packet queue in the mbuf, unless the number of the packet
	 * does not exceed maxqueuelen.  When it exceeds maxqueuelen,
	 * the oldest packet in the queue will be removed.
	 */
	if (ln->ln_state == ND_LLINFO_NOSTATE ||
	    ln->ln_state == ND_LLINFO_WAITDELETE)
		ln->ln_state = ND_LLINFO_INCOMPLETE;

#ifdef MBUFTRACE
	m_claimm(m, ln->lle_tbl->llt_mowner);
#endif
	if (ln->ln_hold != NULL) {
		struct mbuf *m_hold;
		int i;

		i = 0;
		for (m_hold = ln->ln_hold; m_hold; m_hold = m_hold->m_nextpkt) {
			i++;
			if (m_hold->m_nextpkt == NULL) {
				m_hold->m_nextpkt = m;
				break;
			}
		}
		KASSERTMSG(ln->la_numheld == i, "la_numheld=%d i=%d",
		    ln->la_numheld, i);
		while (i >= nd->nd_maxqueuelen) {
			m_hold = ln->ln_hold;
			ln->ln_hold = ln->ln_hold->m_nextpkt;
			m_freem(m_hold);
			i--;
			ln->la_numheld--;
		}
	} else {
		KASSERTMSG(ln->la_numheld == 0, "la_numheld=%d",
		    ln->la_numheld);
		ln->ln_hold = m;
	}

	KASSERTMSG(ln->la_numheld < nd->nd_maxqueuelen,
	    "la_numheld=%d nd_maxqueuelen=%d",
	    ln->la_numheld, nd->nd_maxqueuelen);
	ln->la_numheld++;

	if (ln->ln_asked >= nd->nd_mmaxtries)
		error = (rt != NULL && rt->rt_flags & RTF_GATEWAY) ?
		    EHOSTUNREACH : EHOSTDOWN;
	else
		error = EWOULDBLOCK;

	/*
	 * If there has been no NS for the neighbor after entering the
	 * INCOMPLETE state, send the first solicitation.
	 */
	if (!ND_IS_LLINFO_PERMANENT(ln) && ln->ln_asked == 0) {
		struct psref psref;
		union l3addr dst, src, *psrc;

		ln->ln_asked++;
		nd_set_timer(ln, ND_TIMER_RETRANS);
		memcpy(&dst, &ln->r_l3addr, sizeof(dst));
		psrc = nd->nd_holdsrc(ln, &src);
		if_acquire(ifp, &psref);
		LLE_WUNLOCK(ln);

		nd->nd_output(ifp, NULL, &dst, NULL, psrc);
		if_release(ifp, &psref);
	} else
		LLE_WUNLOCK(ln);

	return error;
}

void
nd_nud_hint(struct llentry *ln)
{
	struct nd_domain *nd;

	if (ln == NULL)
		return;

	LLE_WLOCK_ASSERT(ln);

	if (ln->ln_state < ND_LLINFO_REACHABLE)
		goto done;

	nd = nd_find_domain(ln->lle_tbl->llt_af);

	/*
	 * if we get upper-layer reachability confirmation many times,
	 * it is possible we have false information.
	 */
	ln->ln_byhint++;
	if (ln->ln_byhint > nd->nd_maxnudhint)
		goto done;

	ln->ln_state = ND_LLINFO_REACHABLE;
	if (!ND_IS_LLINFO_PERMANENT(ln))
		nd_set_timer(ln, ND_TIMER_REACHABLE);

done:
	LLE_WUNLOCK(ln);

	return;
}

static struct nd_domain *
nd_find_domain(int af)
{

	KASSERT(af < __arraycount(nd_domains) && nd_domains[af] != NULL);
	return nd_domains[af];
}

void
nd_attach_domain(struct nd_domain *nd)
{

	KASSERT(nd->nd_family < __arraycount(nd_domains));
	nd_domains[nd->nd_family] = nd;
}
