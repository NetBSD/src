/*	$NetBSD: mld6.c,v 1.91.2.2 2018/06/25 07:26:07 pgoyette Exp $	*/
/*	$KAME: mld6.c,v 1.25 2001/01/16 14:14:18 itojun Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	@(#)igmp.c	8.1 (Berkeley) 7/19/93
 */

/*
 * Copyright (c) 1988 Stephen Deering.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)igmp.c	8.1 (Berkeley) 7/19/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mld6.c,v 1.91.2.2 2018/06/25 07:26:07 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/cprng.h>
#include <sys/rwlock.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/icmp6_private.h>
#include <netinet6/mld6_var.h>

static krwlock_t	in6_multilock __cacheline_aligned;

/*
 * Protocol constants
 */

/*
 * time between repetitions of a node's initial report of interest in a
 * multicast address(in seconds)
 */
#define MLD_UNSOLICITED_REPORT_INTERVAL	10

static struct ip6_pktopts ip6_opts;

static void mld_start_listening(struct in6_multi *);
static void mld_stop_listening(struct in6_multi *);

static struct mld_hdr *mld_allocbuf(struct mbuf **, struct in6_multi *, int);
static void mld_sendpkt(struct in6_multi *, int, const struct in6_addr *);
static void mld_starttimer(struct in6_multi *);
static void mld_stoptimer(struct in6_multi *);
static u_long mld_timerresid(struct in6_multi *);

static void in6m_ref(struct in6_multi *);
static void in6m_unref(struct in6_multi *);
static void in6m_destroy(struct in6_multi *);

void
mld_init(void)
{
	static u_int8_t hbh_buf[8];
	struct ip6_hbh *hbh = (struct ip6_hbh *)hbh_buf;
	u_int16_t rtalert_code = htons((u_int16_t)IP6OPT_RTALERT_MLD);

	/* ip6h_nxt will be fill in later */
	hbh->ip6h_len = 0;	/* (8 >> 3) - 1 */

	/* XXX: grotty hard coding... */
	hbh_buf[2] = IP6OPT_PADN;	/* 2 byte padding */
	hbh_buf[3] = 0;
	hbh_buf[4] = IP6OPT_RTALERT;
	hbh_buf[5] = IP6OPT_RTALERT_LEN - 2;
	memcpy(&hbh_buf[6], (void *)&rtalert_code, sizeof(u_int16_t));

	ip6_opts.ip6po_hbh = hbh;
	/* We will specify the hoplimit by a multicast option. */
	ip6_opts.ip6po_hlim = -1;
	ip6_opts.ip6po_prefer_tempaddr = IP6PO_TEMPADDR_NOTPREFER;

	rw_init(&in6_multilock);
}

static void
mld_starttimer(struct in6_multi *in6m)
{
	struct timeval now;

	KASSERT(rw_write_held(&in6_multilock));
	KASSERT(in6m->in6m_timer != IN6M_TIMER_UNDEF);

	microtime(&now);
	in6m->in6m_timer_expire.tv_sec = now.tv_sec + in6m->in6m_timer / hz;
	in6m->in6m_timer_expire.tv_usec = now.tv_usec +
	    (in6m->in6m_timer % hz) * (1000000 / hz);
	if (in6m->in6m_timer_expire.tv_usec > 1000000) {
		in6m->in6m_timer_expire.tv_sec++;
		in6m->in6m_timer_expire.tv_usec -= 1000000;
	}

	/* start or restart the timer */
	callout_schedule(&in6m->in6m_timer_ch, in6m->in6m_timer);
}

/*
 * mld_stoptimer releases in6_multilock when calling callout_halt.
 * The caller must ensure in6m won't be freed while releasing the lock.
 */
static void
mld_stoptimer(struct in6_multi *in6m)
{

	KASSERT(rw_write_held(&in6_multilock));

	if (in6m->in6m_timer == IN6M_TIMER_UNDEF)
		return;

	rw_exit(&in6_multilock);

	callout_halt(&in6m->in6m_timer_ch, NULL);

	rw_enter(&in6_multilock, RW_WRITER);

	in6m->in6m_timer = IN6M_TIMER_UNDEF;
}

static void
mld_timeo(void *arg)
{
	struct in6_multi *in6m = arg;

	KASSERT(in6m->in6m_refcount > 0);

	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	rw_enter(&in6_multilock, RW_WRITER);
	if (in6m->in6m_timer == IN6M_TIMER_UNDEF)
		goto out;

	in6m->in6m_timer = IN6M_TIMER_UNDEF;

	switch (in6m->in6m_state) {
	case MLD_REPORTPENDING:
		mld_start_listening(in6m);
		break;
	default:
		mld_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
		break;
	}

out:
	rw_exit(&in6_multilock);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

static u_long
mld_timerresid(struct in6_multi *in6m)
{
	struct timeval now, diff;

	microtime(&now);

	if (now.tv_sec > in6m->in6m_timer_expire.tv_sec ||
	    (now.tv_sec == in6m->in6m_timer_expire.tv_sec &&
	    now.tv_usec > in6m->in6m_timer_expire.tv_usec)) {
		return (0);
	}
	diff = in6m->in6m_timer_expire;
	diff.tv_sec -= now.tv_sec;
	diff.tv_usec -= now.tv_usec;
	if (diff.tv_usec < 0) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}

	/* return the remaining time in milliseconds */
	return diff.tv_sec * 1000 + diff.tv_usec / 1000;
}

static void
mld_start_listening(struct in6_multi *in6m)
{
	struct in6_addr all_in6;

	KASSERT(rw_write_held(&in6_multilock));

	/*
	 * RFC2710 page 10:
	 * The node never sends a Report or Done for the link-scope all-nodes
	 * address.
	 * MLD messages are never sent for multicast addresses whose scope is 0
	 * (reserved) or 1 (node-local).
	 */
	all_in6 = in6addr_linklocal_allnodes;
	if (in6_setscope(&all_in6, in6m->in6m_ifp, NULL)) {
		/* XXX: this should not happen! */
		in6m->in6m_timer = 0;
		in6m->in6m_state = MLD_OTHERLISTENER;
	}
	if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &all_in6) ||
	    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) < IPV6_ADDR_SCOPE_LINKLOCAL) {
		in6m->in6m_timer = IN6M_TIMER_UNDEF;
		in6m->in6m_state = MLD_OTHERLISTENER;
	} else {
		mld_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
		in6m->in6m_timer = cprng_fast32() %
		    (MLD_UNSOLICITED_REPORT_INTERVAL * hz);
		in6m->in6m_state = MLD_IREPORTEDLAST;

		mld_starttimer(in6m);
	}
}

static void
mld_stop_listening(struct in6_multi *in6m)
{
	struct in6_addr allnode, allrouter;

	KASSERT(rw_lock_held(&in6_multilock));

	allnode = in6addr_linklocal_allnodes;
	if (in6_setscope(&allnode, in6m->in6m_ifp, NULL)) {
		/* XXX: this should not happen! */
		return;
	}
	allrouter = in6addr_linklocal_allrouters;
	if (in6_setscope(&allrouter, in6m->in6m_ifp, NULL)) {
		/* XXX impossible */
		return;
	}

	if (in6m->in6m_state == MLD_IREPORTEDLAST &&
	    (!IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &allnode)) &&
	    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) >
	    IPV6_ADDR_SCOPE_INTFACELOCAL) {
		mld_sendpkt(in6m, MLD_LISTENER_DONE, &allrouter);
	}
}

void
mld_input(struct mbuf *m, int off)
{
	struct ip6_hdr *ip6;
	struct mld_hdr *mldh;
	struct ifnet *ifp;
	struct in6_multi *in6m = NULL;
	struct in6_addr mld_addr, all_in6;
	u_long timer = 0;	/* timer value in the MLD query header */
	struct psref psref;

	ifp = m_get_rcvif_psref(m, &psref);
	if (__predict_false(ifp == NULL))
		goto out;
	IP6_EXTHDR_GET(mldh, struct mld_hdr *, m, off, sizeof(*mldh));
	if (mldh == NULL) {
		ICMP6_STATINC(ICMP6_STAT_TOOSHORT);
		goto out_nodrop;
	}

	ip6 = mtod(m, struct ip6_hdr *);

	/* source address validation */
	if (!IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_src)) {
		/*
		 * RFC3590 allows the IPv6 unspecified address as the source
		 * address of MLD report and done messages.  However, as this
		 * same document says, this special rule is for snooping
		 * switches and the RFC requires routers to discard MLD packets
		 * with the unspecified source address.  The RFC only talks
		 * about hosts receiving an MLD query or report in Security
		 * Considerations, but this is probably the correct intention.
		 * RFC3590 does not talk about other cases than link-local and
		 * the unspecified source addresses, but we believe the same
		 * rule should be applied.
		 * As a result, we only allow link-local addresses as the
		 * source address; otherwise, simply discard the packet.
		 */
#if 0
		/*
		 * XXX: do not log in an input path to avoid log flooding,
		 * though RFC3590 says "SHOULD log" if the source of a query
		 * is the unspecified address.
		 */
		char ip6bufs[INET6_ADDRSTRLEN];
		char ip6bufm[INET6_ADDRSTRLEN];
		log(LOG_INFO,
		    "mld_input: src %s is not link-local (grp=%s)\n",
		    IN6_PRINT(ip6bufs,&ip6->ip6_src),
		    IN6_PRINT(ip6bufm, &mldh->mld_addr));
#endif
		goto out;
	}

	/*
	 * make a copy for local work (in6_setscope() may modify the 1st arg)
	 */
	mld_addr = mldh->mld_addr;
	if (in6_setscope(&mld_addr, ifp, NULL)) {
		/* XXX: this should not happen! */
		goto out;
	}

	/*
	 * In the MLD specification, there are 3 states and a flag.
	 *
	 * In Non-Listener state, we simply don't have a membership record.
	 * In Delaying Listener state, our timer is running (in6m->in6m_timer)
	 * In Idle Listener state, our timer is not running
	 * (in6m->in6m_timer==IN6M_TIMER_UNDEF)
	 *
	 * The flag is in6m->in6m_state, it is set to MLD_OTHERLISTENER if
	 * we have heard a report from another member, or MLD_IREPORTEDLAST
	 * if we sent the last report.
	 */
	switch (mldh->mld_type) {
	case MLD_LISTENER_QUERY: {
		struct in6_multi *next;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN6_IS_ADDR_UNSPECIFIED(&mld_addr) &&
		    !IN6_IS_ADDR_MULTICAST(&mld_addr))
			break;	/* print error or log stat? */

		all_in6 = in6addr_linklocal_allnodes;
		if (in6_setscope(&all_in6, ifp, NULL)) {
			/* XXX: this should not happen! */
			break;
		}

		/*
		 * - Start the timers in all of our membership records
		 *   that the query applies to for the interface on
		 *   which the query arrived excl. those that belong
		 *   to the "all-nodes" group (ff02::1).
		 * - Restart any timer that is already running but has
		 *   a value longer than the requested timeout.
		 * - Use the value specified in the query message as
		 *   the maximum timeout.
		 */
		timer = ntohs(mldh->mld_maxdelay);

		rw_enter(&in6_multilock, RW_WRITER);
		/*
		 * mld_stoptimer and mld_sendpkt release in6_multilock
		 * temporarily, so we have to prevent in6m from being freed
		 * while releasing the lock by having an extra reference to it.
		 *
		 * Also in6_purge_multi might remove items from the list of the
		 * ifp while releasing the lock. Fortunately in6_purge_multi is
		 * never executed as long as we have a psref of the ifp.
		 */
		LIST_FOREACH_SAFE(in6m, &ifp->if_multiaddrs, in6m_entry, next) {
			if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &all_in6) ||
			    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) <
			    IPV6_ADDR_SCOPE_LINKLOCAL)
				continue;

			if (in6m->in6m_state == MLD_REPORTPENDING)
				continue; /* we are not yet ready */

			if (!IN6_IS_ADDR_UNSPECIFIED(&mld_addr) &&
			    !IN6_ARE_ADDR_EQUAL(&mld_addr, &in6m->in6m_addr))
				continue;

			if (timer == 0) {
				in6m_ref(in6m);

				/* send a report immediately */
				mld_stoptimer(in6m);
				mld_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
				in6m->in6m_state = MLD_IREPORTEDLAST;

				in6m_unref(in6m); /* May free in6m */
			} else if (in6m->in6m_timer == IN6M_TIMER_UNDEF ||
			    mld_timerresid(in6m) > timer) {
				in6m->in6m_timer =
				   1 + (cprng_fast32() % timer) * hz / 1000;
				mld_starttimer(in6m);
			}
		}
		rw_exit(&in6_multilock);
		break;
	    }

	case MLD_LISTENER_REPORT:
		/*
		 * For fast leave to work, we have to know that we are the
		 * last person to send a report for this group.  Reports
		 * can potentially get looped back if we are a multicast
		 * router, so discard reports sourced by me.
		 * Note that it is impossible to check IFF_LOOPBACK flag of
		 * ifp for this purpose, since ip6_mloopback pass the physical
		 * interface to looutput.
		 */
		if (m->m_flags & M_LOOP) /* XXX: grotty flag, but efficient */
			break;

		if (!IN6_IS_ADDR_MULTICAST(&mldh->mld_addr))
			break;

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		rw_enter(&in6_multilock, RW_WRITER);
		in6m = in6_lookup_multi(&mld_addr, ifp);
		if (in6m) {
			in6m_ref(in6m);
			mld_stoptimer(in6m); /* transit to idle state */
			in6m->in6m_state = MLD_OTHERLISTENER; /* clear flag */
			in6m_unref(in6m);
			in6m = NULL; /* in6m might be freed */
		}
		rw_exit(&in6_multilock);
		break;
	default:		/* this is impossible */
#if 0
		/*
		 * this case should be impossible because of filtering in
		 * icmp6_input().  But we explicitly disabled this part
		 * just in case.
		 */
		log(LOG_ERR, "mld_input: illegal type(%d)", mldh->mld_type);
#endif
		break;
	}

out:
	m_freem(m);
out_nodrop:
	m_put_rcvif_psref(ifp, &psref);
}

/*
 * XXX mld_sendpkt must be called with in6_multilock held and
 * will release in6_multilock before calling ip6_output and
 * returning to avoid locking against myself in ip6_output.
 */
static void
mld_sendpkt(struct in6_multi *in6m, int type, const struct in6_addr *dst)
{
	struct mbuf *mh;
	struct mld_hdr *mldh;
	struct ip6_hdr *ip6 = NULL;
	struct ip6_moptions im6o;
	struct in6_ifaddr *ia = NULL;
	struct ifnet *ifp = in6m->in6m_ifp;
	int ignflags;
	struct psref psref;
	int bound;

	KASSERT(rw_write_held(&in6_multilock));

	/*
	 * At first, find a link local address on the outgoing interface
	 * to use as the source address of the MLD packet.
	 * We do not reject tentative addresses for MLD report to deal with
	 * the case where we first join a link-local address.
	 */
	ignflags = (IN6_IFF_NOTREADY|IN6_IFF_ANYCAST) & ~IN6_IFF_TENTATIVE;
	bound = curlwp_bind();
	ia = in6ifa_ifpforlinklocal_psref(ifp, ignflags, &psref);
	if (ia == NULL) {
		curlwp_bindx(bound);
		return;
	}
	if ((ia->ia6_flags & IN6_IFF_TENTATIVE)) {
		ia6_release(ia, &psref);
		ia = NULL;
	}

	/* Allocate two mbufs to store IPv6 header and MLD header */
	mldh = mld_allocbuf(&mh, in6m, type);
	if (mldh == NULL) {
		ia6_release(ia, &psref);
		curlwp_bindx(bound);
		return;
	}

	/* fill src/dst here */
	ip6 = mtod(mh, struct ip6_hdr *);
	ip6->ip6_src = ia ? ia->ia_addr.sin6_addr : in6addr_any;
	ip6->ip6_dst = dst ? *dst : in6m->in6m_addr;
	ia6_release(ia, &psref);
	curlwp_bindx(bound);

	mldh->mld_addr = in6m->in6m_addr;
	in6_clearscope(&mldh->mld_addr); /* XXX */
	mldh->mld_cksum = in6_cksum(mh, IPPROTO_ICMPV6, sizeof(struct ip6_hdr),
	    sizeof(struct mld_hdr));

	/* construct multicast option */
	memset(&im6o, 0, sizeof(im6o));
	im6o.im6o_multicast_if_index = if_get_index(ifp);
	im6o.im6o_multicast_hlim = 1;

	/*
	 * Request loopback of the report if we are acting as a multicast
	 * router, so that the process-level routing daemon can hear it.
	 */
	im6o.im6o_multicast_loop = (ip6_mrouter != NULL);

	/* increment output statistics */
	ICMP6_STATINC(ICMP6_STAT_OUTHIST + type);
	icmp6_ifstat_inc(ifp, ifs6_out_msg);
	switch (type) {
	case MLD_LISTENER_QUERY:
		icmp6_ifstat_inc(ifp, ifs6_out_mldquery);
		break;
	case MLD_LISTENER_REPORT:
		icmp6_ifstat_inc(ifp, ifs6_out_mldreport);
		break;
	case MLD_LISTENER_DONE:
		icmp6_ifstat_inc(ifp, ifs6_out_mlddone);
		break;
	}

	/* XXX we cannot call ip6_output with holding in6_multilock */
	rw_exit(&in6_multilock);

	ip6_output(mh, &ip6_opts, NULL, ia ? 0 : IPV6_UNSPECSRC,
	    &im6o, NULL, NULL);

	rw_enter(&in6_multilock, RW_WRITER);
}

static struct mld_hdr *
mld_allocbuf(struct mbuf **mh, struct in6_multi *in6m, int type)
{
	struct mbuf *md;
	struct mld_hdr *mldh;
	struct ip6_hdr *ip6;

	/*
	 * Allocate mbufs to store ip6 header and MLD header.
	 * We allocate 2 mbufs and make chain in advance because
	 * it is more convenient when inserting the hop-by-hop option later.
	 */
	MGETHDR(*mh, M_DONTWAIT, MT_HEADER);
	if (*mh == NULL)
		return NULL;
	MGET(md, M_DONTWAIT, MT_DATA);
	if (md == NULL) {
		m_free(*mh);
		*mh = NULL;
		return NULL;
	}
	(*mh)->m_next = md;
	md->m_next = NULL;

	m_reset_rcvif((*mh));
	(*mh)->m_pkthdr.len = sizeof(struct ip6_hdr) + sizeof(struct mld_hdr);
	(*mh)->m_len = sizeof(struct ip6_hdr);
	MH_ALIGN(*mh, sizeof(struct ip6_hdr));

	/* fill in the ip6 header */
	ip6 = mtod(*mh, struct ip6_hdr *);
	memset(ip6, 0, sizeof(*ip6));
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	/* ip6_plen will be set later */
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	/* ip6_hlim will be set by im6o.im6o_multicast_hlim */
	/* ip6_src/dst will be set by mld_sendpkt() or mld_sendbuf() */

	/* fill in the MLD header as much as possible */
	md->m_len = sizeof(struct mld_hdr);
	mldh = mtod(md, struct mld_hdr *);
	memset(mldh, 0, sizeof(struct mld_hdr));
	mldh->mld_type = type;
	return mldh;
}

static void
in6m_ref(struct in6_multi *in6m)
{

	KASSERT(rw_write_held(&in6_multilock));
	in6m->in6m_refcount++;
}

static void
in6m_unref(struct in6_multi *in6m)
{

	KASSERT(rw_write_held(&in6_multilock));
	if (--in6m->in6m_refcount == 0)
		in6m_destroy(in6m);
}

/*
 * Add an address to the list of IP6 multicast addresses for a given interface.
 */
struct	in6_multi *
in6_addmulti(struct in6_addr *maddr6, struct ifnet *ifp, int *errorp,
    int timer)
{
	struct	sockaddr_in6 sin6;
	struct	in6_multi *in6m;

	*errorp = 0;

	rw_enter(&in6_multilock, RW_WRITER);
	/*
	 * See if address already in list.
	 */
	in6m = in6_lookup_multi(maddr6, ifp);
	if (in6m != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		in6m->in6m_refcount++;
	} else {
		/*
		 * New address; allocate a new multicast record
		 * and link it into the interface's multicast list.
		 */
		in6m = malloc(sizeof(*in6m), M_IPMADDR, M_NOWAIT|M_ZERO);
		if (in6m == NULL) {
			*errorp = ENOBUFS;
			goto out;
		}

		in6m->in6m_addr = *maddr6;
		in6m->in6m_ifp = ifp;
		in6m->in6m_refcount = 1;
		in6m->in6m_timer = IN6M_TIMER_UNDEF;
		callout_init(&in6m->in6m_timer_ch, CALLOUT_MPSAFE);
		callout_setfunc(&in6m->in6m_timer_ch, mld_timeo, in6m);

		LIST_INSERT_HEAD(&ifp->if_multiaddrs, in6m, in6m_entry);

		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		sockaddr_in6_init(&sin6, maddr6, 0, 0, 0);
		*errorp = if_mcast_op(ifp, SIOCADDMULTI, sin6tosa(&sin6));
		if (*errorp) {
			callout_destroy(&in6m->in6m_timer_ch);
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			in6m = NULL;
			goto out;
		}

		in6m->in6m_timer = timer;
		if (in6m->in6m_timer > 0) {
			in6m->in6m_state = MLD_REPORTPENDING;
			mld_starttimer(in6m);
			goto out;
		}

		/*
		 * Let MLD6 know that we have joined a new IP6 multicast
		 * group.
		 */
		mld_start_listening(in6m);
	}
out:
	rw_exit(&in6_multilock);
	return in6m;
}

static void
in6m_destroy(struct in6_multi *in6m)
{
	struct sockaddr_in6 sin6;

	KASSERT(rw_write_held(&in6_multilock));
	KASSERT(in6m->in6m_refcount == 0);

	/*
	 * Unlink from list if it's listed.  This must be done before
	 * mld_stop_listening because it releases in6_multilock and that allows
	 * someone to look up the removing in6m from the list and add a
	 * reference to the entry unexpectedly.
	 */
	if (in6_lookup_multi(&in6m->in6m_addr, in6m->in6m_ifp) != NULL)
		LIST_REMOVE(in6m, in6m_entry);

	/*
	 * No remaining claims to this record; let MLD6 know
	 * that we are leaving the multicast group.
	 */
	mld_stop_listening(in6m);

	/*
	 * Delete all references of this multicasting group from
	 * the membership arrays
	 */
	in6_purge_mcast_references(in6m);

	/*
	 * Notify the network driver to update its multicast
	 * reception filter.
	 */
	sockaddr_in6_init(&sin6, &in6m->in6m_addr, 0, 0, 0);
	if_mcast_op(in6m->in6m_ifp, SIOCDELMULTI, sin6tosa(&sin6));

	/* Tell mld_timeo we're halting the timer */
	in6m->in6m_timer = IN6M_TIMER_UNDEF;

	rw_exit(&in6_multilock);
	callout_halt(&in6m->in6m_timer_ch, NULL);
	callout_destroy(&in6m->in6m_timer_ch);

	free(in6m, M_IPMADDR);
	rw_enter(&in6_multilock, RW_WRITER);
}

/*
 * Delete a multicast address record.
 */
void
in6_delmulti_locked(struct in6_multi *in6m)
{

	KASSERT(rw_write_held(&in6_multilock));
	KASSERT(in6m->in6m_refcount > 0);

	/*
	 * The caller should have a reference to in6m. So we don't need to care
	 * of releasing the lock in mld_stoptimer.
	 */
	mld_stoptimer(in6m);
	if (--in6m->in6m_refcount == 0)
		in6m_destroy(in6m);
}

void
in6_delmulti(struct in6_multi *in6m)
{

	rw_enter(&in6_multilock, RW_WRITER);
	in6_delmulti_locked(in6m);
	rw_exit(&in6_multilock);
}

/*
 * Look up the in6_multi record for a given IP6 multicast address
 * on a given interface. If no matching record is found, "in6m"
 * returns NULL.
 */
struct in6_multi *
in6_lookup_multi(const struct in6_addr *addr, const struct ifnet *ifp)
{
	struct in6_multi *in6m;

	KASSERT(rw_lock_held(&in6_multilock));

	LIST_FOREACH(in6m, &ifp->if_multiaddrs, in6m_entry) {
		if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, addr))
			break;
	}
	return in6m;
}

void
in6_lookup_and_delete_multi(const struct in6_addr *addr,
    const struct ifnet *ifp)
{
	struct in6_multi *in6m;

	rw_enter(&in6_multilock, RW_WRITER);
	in6m = in6_lookup_multi(addr, ifp);
	if (in6m != NULL)
		in6_delmulti_locked(in6m);
	rw_exit(&in6_multilock);
}

bool
in6_multi_group(const struct in6_addr *addr, const struct ifnet *ifp)
{
	bool ingroup;

	rw_enter(&in6_multilock, RW_READER);
	ingroup = in6_lookup_multi(addr, ifp) != NULL;
	rw_exit(&in6_multilock);

	return ingroup;
}

/*
 * Purge in6_multi records associated to the interface.
 */
void
in6_purge_multi(struct ifnet *ifp)
{
	struct in6_multi *in6m, *next;

	rw_enter(&in6_multilock, RW_WRITER);
	LIST_FOREACH_SAFE(in6m, &ifp->if_multiaddrs, in6m_entry, next) {
		LIST_REMOVE(in6m, in6m_entry);
		/*
		 * Normally multicast addresses are already purged at this
		 * point. Remaining references aren't accessible via ifp,
		 * so what we can do here is to prevent ifp from being
		 * accessed via in6m by removing it from the list of ifp.
		 */
		mld_stoptimer(in6m);
	}
	rw_exit(&in6_multilock);
}

void
in6_multi_lock(int op)
{

	rw_enter(&in6_multilock, op);
}

void
in6_multi_unlock(void)
{

	rw_exit(&in6_multilock);
}

bool
in6_multi_locked(int op)
{

	switch (op) {
	case RW_READER:
		return rw_read_held(&in6_multilock);
	case RW_WRITER:
		return rw_write_held(&in6_multilock);
	default:
		return rw_lock_held(&in6_multilock);
	}
}

struct in6_multi_mship *
in6_joingroup(struct ifnet *ifp, struct in6_addr *addr, int *errorp, int timer)
{
	struct in6_multi_mship *imm;

	imm = malloc(sizeof(*imm), M_IPMADDR, M_NOWAIT|M_ZERO);
	if (imm == NULL) {
		*errorp = ENOBUFS;
		return NULL;
	}

	imm->i6mm_maddr = in6_addmulti(addr, ifp, errorp, timer);
	if (!imm->i6mm_maddr) {
		/* *errorp is already set */
		free(imm, M_IPMADDR);
		return NULL;
	}
	return imm;
}

int
in6_leavegroup(struct in6_multi_mship *imm)
{
	struct in6_multi *in6m;

	rw_enter(&in6_multilock, RW_WRITER);
	in6m = imm->i6mm_maddr;
	imm->i6mm_maddr = NULL;
	if (in6m != NULL) {
		in6_delmulti_locked(in6m);
	}
	rw_exit(&in6_multilock);
	free(imm, M_IPMADDR);
	return 0;
}

/*
 * DEPRECATED: keep it just to avoid breaking old sysctl users.
 */
static int
in6_mkludge_sysctl(SYSCTLFN_ARGS)
{

	if (namelen != 1)
		return EINVAL;
	*oldlenp = 0;
	return 0;
}

static int
in6_multicast_sysctl(SYSCTLFN_ARGS)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct in6_ifaddr *ia6;
	struct in6_multi *in6m;
	uint32_t tmp;
	int error;
	size_t written;
	struct psref psref, psref_ia;
	int bound, s;

	if (namelen != 1)
		return EINVAL;

	rw_enter(&in6_multilock, RW_READER);

	bound = curlwp_bind();
	ifp = if_get_byindex(name[0], &psref);
	if (ifp == NULL) {
		curlwp_bindx(bound);
		rw_exit(&in6_multilock);
		return ENODEV;
	}

	if (oldp == NULL) {
		*oldlenp = 0;
		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			LIST_FOREACH(in6m, &ifp->if_multiaddrs, in6m_entry) {
				*oldlenp += 2 * sizeof(struct in6_addr) +
				    sizeof(uint32_t);
			}
		}
		pserialize_read_exit(s);
		if_put(ifp, &psref);
		curlwp_bindx(bound);
		rw_exit(&in6_multilock);
		return 0;
	}

	error = 0;
	written = 0;
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		ifa_acquire(ifa, &psref_ia);
		pserialize_read_exit(s);

		ia6 = ifatoia6(ifa);
		LIST_FOREACH(in6m, &ifp->if_multiaddrs, in6m_entry) {
			if (written + 2 * sizeof(struct in6_addr) +
			    sizeof(uint32_t) > *oldlenp)
				goto done;
			/*
			 * XXX return the first IPv6 address to keep backward
			 * compatibility, however now multicast addresses
			 * don't belong to any IPv6 addresses so it should be
			 * unnecessary.
			 */
			error = sysctl_copyout(l, &ia6->ia_addr.sin6_addr,
			    oldp, sizeof(struct in6_addr));
			if (error)
				goto done;
			oldp = (char *)oldp + sizeof(struct in6_addr);
			written += sizeof(struct in6_addr);
			error = sysctl_copyout(l, &in6m->in6m_addr,
			    oldp, sizeof(struct in6_addr));
			if (error)
				goto done;
			oldp = (char *)oldp + sizeof(struct in6_addr);
			written += sizeof(struct in6_addr);
			tmp = in6m->in6m_refcount;
			error = sysctl_copyout(l, &tmp, oldp, sizeof(tmp));
			if (error)
				goto done;
			oldp = (char *)oldp + sizeof(tmp);
			written += sizeof(tmp);
		}

		s = pserialize_read_enter();

		break;
	}
	pserialize_read_exit(s);
done:
	ifa_release(ifa, &psref_ia);
	if_put(ifp, &psref);
	curlwp_bindx(bound);
	rw_exit(&in6_multilock);
	*oldlenp = written;
	return error;
}

void
in6_sysctl_multicast_setup(struct sysctllog **clog)
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet6", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "multicast",
		       SYSCTL_DESCR("Multicast information"),
		       in6_multicast_sysctl, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "multicast_kludge",
		       SYSCTL_DESCR("multicast kludge information"),
		       in6_mkludge_sysctl, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_CREATE, CTL_EOL);
}
