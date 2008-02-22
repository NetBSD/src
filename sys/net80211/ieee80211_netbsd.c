/* $NetBSD: ieee80211_netbsd.c,v 1.16.4.1 2008/02/22 16:50:25 skrll Exp $ */
/*-
 * Copyright (c) 2003-2005 Sam Leffler, Errno Consulting
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/net80211/ieee80211_freebsd.c,v 1.8 2005/08/08 18:46:35 sam Exp $");
#else
__KERNEL_RCSID(0, "$NetBSD: ieee80211_netbsd.c,v 1.16.4.1 2008/02/22 16:50:25 skrll Exp $");
#endif

/*
 * IEEE 802.11 support (NetBSD-specific code)
 */
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/once.h>

#include <machine/stdarg.h>

#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/route.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_sysctl.h>
#include <net80211/ieee80211_netbsd.h>

#define	LOGICALLY_EQUAL(x, y)	(!(x) == !(y))

static void ieee80211_sysctl_fill_node(struct ieee80211_node *,
    struct ieee80211_node_sysctl *, int, const struct ieee80211_channel *);
static int ieee80211_sysctl_node(SYSCTLFN_ARGS);

#ifdef IEEE80211_DEBUG
int	ieee80211_debug = 0;
#endif

typedef void (*ieee80211_setup_func)(void);

__link_set_decl(ieee80211_funcs, ieee80211_setup_func);

static int
ieee80211_init0(void)
{
	ieee80211_setup_func * const *ieee80211_setup, f;

        __link_set_foreach(ieee80211_setup, ieee80211_funcs) {
		f = (void*)*ieee80211_setup;
		(*f)();
	}

	return 0;
}

void
net80211_init(void)
{
	static ONCE_DECL(ieee80211_init_once);

	RUN_ONCE(&ieee80211_init_once, ieee80211_init0);
}

static int
ieee80211_sysctl_inact(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	/* sysctl_lookup copies the product from t.  Then, it
	 * copies the new value onto t.
	 */
	t = *(int*)rnode->sysctl_data * IEEE80211_INACT_WAIT;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	/* The new value was in seconds.  Convert to inactivity-wait
	 * intervals.  There are IEEE80211_INACT_WAIT seconds per
	 * interval.
	 */
	*(int*)rnode->sysctl_data = t / IEEE80211_INACT_WAIT;

	return (0);
}

static int
ieee80211_sysctl_parent(SYSCTLFN_ARGS)
{
	struct ieee80211com *ic;
	char pname[IFNAMSIZ];
	struct sysctlnode node;

	node = *rnode;
	ic = node.sysctl_data;
	strncpy(pname, ic->ic_ifp->if_xname, IFNAMSIZ);
	node.sysctl_data = pname;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/*
 * Create or get top of sysctl tree net.link.ieee80211.
 */
static const struct sysctlnode *
ieee80211_sysctl_treetop(struct sysctllog **log)
{
	int rc;
	const struct sysctlnode *rnode;

	if ((rc = sysctl_createv(log, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "net", NULL,
	    NULL, 0, NULL, 0, CTL_NET, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(log, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "link",
	    "link-layer statistics and controls",
	    NULL, 0, NULL, 0, PF_LINK, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(log, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "ieee80211",
	    "IEEE 802.11 WLAN statistics and controls",
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return rnode;
err:
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
	return NULL;
}

void
ieee80211_sysctl_attach(struct ieee80211com *ic)
{
	int rc;
	const struct sysctlnode *cnode, *rnode;
	char num[sizeof("vap") + 14];		/* sufficient for 32 bits */

	if ((rnode = ieee80211_sysctl_treetop(NULL)) == NULL)
		return;

	snprintf(num, sizeof(num), "vap%u", ic->ic_vap);

	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, num, SYSCTL_DESCR("virtual AP"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	/* control debugging printfs */
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING,
	    "parent", SYSCTL_DESCR("parent device"),
	    ieee80211_sysctl_parent, 0, ic, IFNAMSIZ, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

#ifdef IEEE80211_DEBUG
	/* control debugging printfs */
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("control debugging printfs"),
	    NULL, ieee80211_debug, &ic->ic_debug, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif
	/* XXX inherit from tunables */
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "inact_run", SYSCTL_DESCR("station inactivity timeout (sec)"),
	    ieee80211_sysctl_inact, 0, &ic->ic_inact_run, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "inact_probe",
	    SYSCTL_DESCR("station inactivity probe timeout (sec)"),
	    ieee80211_sysctl_inact, 0, &ic->ic_inact_probe, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "inact_auth",
	    SYSCTL_DESCR("station authentication timeout (sec)"),
	    ieee80211_sysctl_inact, 0, &ic->ic_inact_auth, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "inact_init",
	    SYSCTL_DESCR("station initial state timeout (sec)"),
	    ieee80211_sysctl_inact, 0, &ic->ic_inact_init, 0,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "driver_caps", SYSCTL_DESCR("driver capabilities"),
	    NULL, 0, &ic->ic_caps, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	if ((rc = sysctl_createv(&ic->ic_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "bmiss_max", SYSCTL_DESCR("consecutive beacon misses before scanning"),
	    NULL, 0, &ic->ic_bmiss_max, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return;
err:
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
}

void
ieee80211_sysctl_detach(struct ieee80211com *ic)
{
	sysctl_teardown(&ic->ic_sysctllog);
}

static void
ieee80211_sysctl_fill_node(struct ieee80211_node *ni,
    struct ieee80211_node_sysctl *ns, int ifindex,
    const struct ieee80211_channel *chan0)
{
	ns->ns_ifindex = ifindex;
	ns->ns_capinfo = ni->ni_capinfo;
	ns->ns_flags |= IEEE80211_NODE_SYSCTL_F_STA;
	(void)memcpy(ns->ns_macaddr, ni->ni_macaddr, sizeof(ns->ns_macaddr));
	(void)memcpy(ns->ns_bssid, ni->ni_bssid, sizeof(ns->ns_bssid));
	if (ni->ni_chan != IEEE80211_CHAN_ANYC) {
		ns->ns_freq = ni->ni_chan->ic_freq;
		ns->ns_chanflags = ni->ni_chan->ic_flags;
		ns->ns_chanidx = ni->ni_chan - chan0;
	} else {
		ns->ns_freq = ns->ns_chanflags = 0;
		ns->ns_chanidx = 0;
	}
	ns->ns_rssi = ni->ni_rssi;
	ns->ns_esslen = ni->ni_esslen;
	(void)memcpy(ns->ns_essid, ni->ni_essid, sizeof(ns->ns_essid));
	ns->ns_erp = ni->ni_erp;
	ns->ns_associd = ni->ni_associd;
	ns->ns_inact = ni->ni_inact * IEEE80211_INACT_WAIT;
	ns->ns_rstamp = ni->ni_rstamp;
	ns->ns_rates = ni->ni_rates;
	ns->ns_txrate = ni->ni_txrate;
	ns->ns_intval = ni->ni_intval;
	(void)memcpy(ns->ns_tstamp, &ni->ni_tstamp, sizeof(ns->ns_tstamp));
	ns->ns_txseq = ni->ni_txseqs[0];
	ns->ns_rxseq = ni->ni_rxseqs[0];
	ns->ns_fhdwell = ni->ni_fhdwell;
	ns->ns_fhindex = ni->ni_fhindex;
	ns->ns_fails = ni->ni_fails;
}

static void
sysctl_read_scan(void *arg, const struct ieee80211_scan_entry *se)
{
	struct ieee80211_node_sysctl ns;
	struct ieee80211_node_walk *nw = arg;
	struct ieee80211com *ic = nw->nw_ic;
	int error, nr, nxr;

	if (nw->nw_error != 0 || nw->nw_nelt <= 0)
		return;

	if (nw->nw_len >= nw->nw_eltsize) {
		memset(&ns, 0, sizeof(ns));
		ns.ns_ifindex = ic->ic_ifp->if_index;
	        IEEE80211_ADDR_COPY(ns.ns_bssid, se->se_bssid);
		ns.ns_capinfo = se->se_capinfo;
		ns.ns_flags = IEEE80211_NODE_SYSCTL_F_SCAN;
		ns.ns_freq = se->se_chan->ic_freq;
		ns.ns_chanflags = se->se_chan->ic_flags;
		ns.ns_chanidx = &ic->ic_channels[0] - se->se_chan; /* XXXNH */
		ns.ns_rssi = se->se_rssi;
		ns.ns_esslen = se->se_ssid[1];
		(void)memcpy(ns.ns_essid, se->se_ssid+2, ns.ns_esslen);
		ns.ns_erp = se->se_erp;
		ns.ns_rstamp = se->se_rstamp;
		nr = min(se->se_rates[1], IEEE80211_RATE_MAXSIZE);
		memcpy(ns.ns_rates.rs_rates, se->se_rates+2, nr);
		nxr = min(se->se_xrates[1], IEEE80211_RATE_MAXSIZE - nr);
		memcpy(ns.ns_rates.rs_rates+nr, se->se_xrates+2, nxr);
		ns.ns_rates.rs_nrates = nr + nxr;
		ns.ns_intval = se->se_intval;
		(void)memcpy(ns.ns_tstamp, &se->se_tstamp,
		    sizeof(ns.ns_tstamp));
		ns.ns_fhdwell = se->se_fhdwell;
		ns.ns_fhindex = se->se_fhindex;

		error = copyout(&ns, nw->nw_dp, nw->nw_out_size);
		if (error) {
			nw->nw_error = error;
			return;
		}
		nw->nw_dp += nw->nw_eltsize;
		nw->nw_len -= nw->nw_eltsize;
	}

	nw->nw_needed += nw->nw_eltsize;
	if (nw->nw_nelt != INT_MAX)
		nw->nw_nelt--;
}
		
static void
sysctl_read_sta(void *arg, struct ieee80211_node *ni)
{
	struct ieee80211_node_sysctl ns;
	struct ieee80211_node_walk *nw = arg;
	struct ieee80211com *ic = nw->nw_ic;
	int error;

	if (nw->nw_error != 0 || nw->nw_nelt <= 0)
		return;

	if (nw->nw_len >= nw->nw_eltsize) {
		ieee80211_sysctl_fill_node(ni, &ns, ic->ic_ifp->if_index,
		    &ic->ic_channels[0]);
		error = copyout(&ns, nw->nw_dp, nw->nw_out_size);
		if (error) {
			nw->nw_error = error;
			return;
		}
		nw->nw_dp += nw->nw_eltsize;
		nw->nw_len -= nw->nw_eltsize;
	}

	nw->nw_needed += nw->nw_eltsize;
	if (nw->nw_nelt != INT_MAX)
		nw->nw_nelt--;
}

/* Between two examinations of the sysctl tree, I expect each
 * interface to add no more than 5 nodes.
 */
#define IEEE80211_SYSCTL_NODE_GROWTH	5

static int
ieee80211_sysctl_node(SYSCTLFN_ARGS)
{
	struct ieee80211_node_walk nw;
	u_int op, arg, hdr_type;
	int s, nelt;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (namelen != IEEE80211_SYSCTL_NODENAMELEN)
		return (EINVAL);

	/* ifindex.op.arg.header-type.eltsize.nelt */
	nw.nw_dp = oldp;
	nw.nw_len = (oldp != NULL) ? *oldlenp : 0;
	nw.nw_ifindex = name[IEEE80211_SYSCTL_NODENAME_IF];
	op = name[IEEE80211_SYSCTL_NODENAME_OP];
	arg = name[IEEE80211_SYSCTL_NODENAME_ARG];
	hdr_type = name[IEEE80211_SYSCTL_NODENAME_TYPE];
	nw.nw_eltsize = name[IEEE80211_SYSCTL_NODENAME_ELTSIZE];
	nelt = name[IEEE80211_SYSCTL_NODENAME_ELTCOUNT];
	nw.nw_out_size = MIN(sizeof(struct ieee80211_node_sysctl), nw.nw_eltsize);

	if (op != IEEE80211_SYSCTL_OP_ALL || arg != 0 ||
	    hdr_type != IEEE80211_SYSCTL_T_NODE || nw.nw_eltsize < 1 ||
	    nelt < 0)
		return (EINVAL);

	nw.nw_nelt = nelt;
	nw.nw_error = 0;
	nw.nw_needed = 0;
	nw.nw_ifcount = 0;

	s = splnet();
        SLIST_FOREACH(nw.nw_ic, &ieee80211_list, ic_next) {
		if (nw.nw_ifindex != nw.nw_ic->ic_ifp->if_index)
			continue;

		ieee80211_iterate_nodes(&nw.nw_ic->ic_sta, sysctl_read_sta,
		    &nw);
		if (nw.nw_error)
			break;

		ieee80211_scan_iterate(nw.nw_ic, sysctl_read_scan,
		    &nw);
		if (nw.nw_error)
			break;

		/* XXXNH flags */
		if (nw.nw_ic->ic_bss != NULL)
			sysctl_read_sta(&nw, nw.nw_ic->ic_bss);
		if (nw.nw_error)
			break;
	}
	splx(s);

	*oldlenp = nw.nw_needed;
	if (oldp == NULL)
		*oldlenp += nw.nw_ifcount * IEEE80211_SYSCTL_NODE_GROWTH * nw.nw_eltsize;

	return (nw.nw_error);
}

/*
 * Setup sysctl(3) MIB, net.ieee80211.*
 *
 * TBD condition CTLFLAG_PERMANENT on being an LKM or not
 */
SYSCTL_SETUP(sysctl_ieee80211, "sysctl ieee80211 subtree setup")
{
	int rc;
	const struct sysctlnode *cnode, *rnode;

	if ((rnode = ieee80211_sysctl_treetop(clog)) == NULL)
		return;

	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "nodes", "client/peer stations",
	    ieee80211_sysctl_node, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

#ifdef IEEE80211_DEBUG
	/* control debugging printfs */
	if ((rc = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("control debugging printfs"),
	    NULL, 0, &ieee80211_debug, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif /* IEEE80211_DEBUG */

	return;
err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

int
ieee80211_node_dectestref(struct ieee80211_node *ni)
{
	int rc, s;
	s = splnet();
	if (--ni->ni_refcnt == 0) {
		rc = 1;
		ni->ni_refcnt = 1;
	} else
		rc = 0;
	splx(s);
	return rc;
}

void
ieee80211_drain_ifq(struct ifqueue *ifq)
{
	struct ieee80211_node *ni;
	struct mbuf *m;

	for (;;) {
		IF_DEQUEUE(ifq, m);
		if (m == NULL)
			break;

		ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;
		KASSERT(ni != NULL);
		ieee80211_free_node(ni);
		m->m_pkthdr.rcvif = NULL;

		m_freem(m);
	}
}


void
if_printf(struct ifnet *ifp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	printf("%s: ", ifp->if_xname);
	vprintf(fmt, ap);

	va_end(ap);
	return;
}

/*
 * Set the m_data pointer of a newly-allocated mbuf
 * to place an object of the specified size at the
 * end of the mbuf, longword aligned.
 */
void
m_align(struct mbuf *m, int len)
{
       int adjust;

       if (m->m_flags & M_EXT)
	       adjust = m->m_ext.ext_size - len;
       else if (m->m_flags & M_PKTHDR)
	       adjust = MHLEN - len;
       else
	       adjust = MLEN - len;
       m->m_data += adjust &~ (sizeof(long)-1);
}

/*
 * Append the specified data to the indicated mbuf chain,
 * Extend the mbuf chain if the new data does not fit in
 * existing space.
 *
 * Return 1 if able to complete the job; otherwise 0.
 */
int
m_append(struct mbuf *m0, int len, const void *cpv)
{
	struct mbuf *m, *n;
	int remainder, space;
	const char *cp = cpv;

	for (m = m0; m->m_next != NULL; m = m->m_next)
		continue;
	remainder = len;
	space = M_TRAILINGSPACE(m);
	if (space > 0) {
		/*
		 * Copy into available space.
		 */
		if (space > remainder)
			space = remainder;
		memmove(mtod(m, char *) + m->m_len, cp, space);
		m->m_len += space;
		cp = cp + space, remainder -= space;
	}
	while (remainder > 0) {
		/*
		 * Allocate a new mbuf; could check space
		 * and allocate a cluster instead.
		 */
		n = m_get(M_DONTWAIT, m->m_type);
		if (n == NULL)
			break;
		n->m_len = min(MLEN, remainder);
		memmove(mtod(n, void *), cp, n->m_len);
		cp += n->m_len, remainder -= n->m_len;
		m->m_next = n;
		m = n;
	}
	if (m0->m_flags & M_PKTHDR)
		m0->m_pkthdr.len += len - remainder;
	return (remainder == 0);
}

/*
 * Allocate and setup a management frame of the specified
 * size.  We return the mbuf and a pointer to the start
 * of the contiguous data area that's been reserved based
 * on the packet length.  The data area is forced to 32-bit
 * alignment and the buffer length to a multiple of 4 bytes.
 * This is done mainly so beacon frames (that require this)
 * can use this interface too.
 */
struct mbuf *
ieee80211_getmgtframe(u_int8_t **frm, int headroom, u_int pktlen)
{
	struct mbuf *m;
	u_int len;

	/*
	 * NB: we know the mbuf routines will align the data area
	 *     so we don't need to do anything special.
	 */
	/* XXX 4-address frame? */
	len = roundup(sizeof(struct ieee80211_frame) + pktlen, 4);
	IASSERT(len <= MCLBYTES, ("802.11 mgt frame too large: %u", len));
	if (len <= MHLEN) {
		m = m_gethdr(M_NOWAIT, MT_HEADER);
		/*
		 * Align the data in case additional headers are added.
		 * This should only happen when a WEP header is added
		 * which only happens for shared key authentication mgt
		 * frames which all fit in MHLEN.
		 */
		if (m != NULL)
			MH_ALIGN(m, len);
	} else
		m = m_getcl(M_NOWAIT, MT_HEADER, M_PKTHDR);
	if (m != NULL) {
		m->m_data += sizeof(struct ieee80211_frame);
		*frm = m->m_data;
		IASSERT((uintptr_t)*frm % 4 == 0, ("bad beacon boundary"));
	}
	return m;
}

void
get_random_bytes(void *p, size_t n)
{
	u_int8_t *dp = p;

	while (n > 0) {
		u_int32_t v = arc4random();
		size_t nb = n > sizeof(u_int32_t) ? sizeof(u_int32_t) : n;
		(void)memcpy(dp, &v, nb);
		dp += sizeof(u_int32_t), n -= nb;
	}
}

void
ieee80211_notify_node_join(struct ieee80211com *ic, struct ieee80211_node *ni, int newassoc)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct ieee80211_join_event iev;

	IEEE80211_DPRINTF(ic, IEEE80211_MSG_NODE, "%s: %snode %s join\n",
	    ifp->if_xname, (ni == ic->ic_bss) ? "bss " : "",
	    ether_sprintf(ni->ni_macaddr));

	memset(&iev, 0, sizeof(iev));
	if (ni == ic->ic_bss) {
		IEEE80211_ADDR_COPY(iev.iev_addr, ni->ni_bssid);
		rt_ieee80211msg(ifp, newassoc ?
			RTM_IEEE80211_ASSOC : RTM_IEEE80211_REASSOC,
			&iev, sizeof(iev));
		if_link_state_change(ifp, LINK_STATE_UP);
	} else {
		IEEE80211_ADDR_COPY(iev.iev_addr, ni->ni_macaddr);
		rt_ieee80211msg(ifp, newassoc ?
		    RTM_IEEE80211_JOIN : RTM_IEEE80211_REJOIN,
		    &iev, sizeof(iev));
	}
}

void
ieee80211_notify_node_leave(struct ieee80211com *ic, struct ieee80211_node *ni)
{
	struct ifnet *ifp = ic->ic_ifp;
	struct ieee80211_leave_event iev;

	IEEE80211_DPRINTF(ic, IEEE80211_MSG_NODE, "%s: %snode %s leave\n",
	    ifp->if_xname, (ni == ic->ic_bss) ? "bss " : "",
	    ether_sprintf(ni->ni_macaddr));

	if (ni == ic->ic_bss) {
		rt_ieee80211msg(ifp, RTM_IEEE80211_DISASSOC, NULL, 0);
		if_link_state_change(ifp, LINK_STATE_DOWN);
	} else {
		/* fire off wireless event station leaving */
		memset(&iev, 0, sizeof(iev));
		IEEE80211_ADDR_COPY(iev.iev_addr, ni->ni_macaddr);
		rt_ieee80211msg(ifp, RTM_IEEE80211_LEAVE, &iev, sizeof(iev));
	}
}

void
ieee80211_notify_scan_done(struct ieee80211com *ic)
{
	struct ifnet *ifp = ic->ic_ifp;

	IEEE80211_DPRINTF(ic, IEEE80211_MSG_SCAN,
		"%s: notify scan done\n", ic->ic_ifp->if_xname);

	/* dispatch wireless event indicating scan completed */
	rt_ieee80211msg(ifp, RTM_IEEE80211_SCAN, NULL, 0);
}

void
ieee80211_notify_replay_failure(struct ieee80211com *ic,
	const struct ieee80211_frame *wh, const struct ieee80211_key *k,
	u_int64_t rsc)
{
	struct ifnet *ifp = ic->ic_ifp;

	IEEE80211_DPRINTF(ic, IEEE80211_MSG_CRYPTO,
	    "[%s] %s replay detected <rsc %ju, csc %ju, keyix %u rxkeyix %u>\n",
	    ether_sprintf(wh->i_addr2), k->wk_cipher->ic_name,
	    (intmax_t) rsc, (intmax_t) k->wk_keyrsc,
	    k->wk_keyix, k->wk_rxkeyix);

	if (ifp != NULL) {		/* NB: for cipher test modules */
		struct ieee80211_replay_event iev;

		IEEE80211_ADDR_COPY(iev.iev_dst, wh->i_addr1);
		IEEE80211_ADDR_COPY(iev.iev_src, wh->i_addr2);
		iev.iev_cipher = k->wk_cipher->ic_cipher;
		if (k->wk_rxkeyix != IEEE80211_KEYIX_NONE)
			iev.iev_keyix = k->wk_rxkeyix;
		else
			iev.iev_keyix = k->wk_keyix;
		iev.iev_keyrsc = k->wk_keyrsc;
		iev.iev_rsc = rsc;
		rt_ieee80211msg(ifp, RTM_IEEE80211_REPLAY, &iev, sizeof(iev));
	}
}

void
ieee80211_notify_michael_failure(struct ieee80211com *ic,
	const struct ieee80211_frame *wh, u_int keyix)
{
	struct ifnet *ifp = ic->ic_ifp;

	IEEE80211_DPRINTF(ic, IEEE80211_MSG_CRYPTO,
		"[%s] michael MIC verification failed <keyix %u>\n",
	       ether_sprintf(wh->i_addr2), keyix);
	ic->ic_stats.is_rx_tkipmic++;

	if (ifp != NULL) {		/* NB: for cipher test modules */
		struct ieee80211_michael_event iev;

		IEEE80211_ADDR_COPY(iev.iev_dst, wh->i_addr1);
		IEEE80211_ADDR_COPY(iev.iev_src, wh->i_addr2);
		iev.iev_cipher = IEEE80211_CIPHER_TKIP;
		iev.iev_keyix = keyix;
		rt_ieee80211msg(ifp, RTM_IEEE80211_MICHAEL, &iev, sizeof(iev));
	}
}

void
ieee80211_load_module(const char *modname)
{
#ifdef notyet
	struct thread *td = curthread;

	if (suser(td) == 0 && securelevel_gt(td->td_ucred, 0) == 0) {
		mtx_lock(&Giant);
		(void) linker_load_module(modname, NULL, NULL, NULL, NULL);
		mtx_unlock(&Giant);
	}
#else
	printf("%s: load the %s module by hand for now.\n", __func__, modname);
#endif
}
