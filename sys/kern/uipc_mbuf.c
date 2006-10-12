/*	$NetBSD: uipc_mbuf.c,v 1.115 2006/10/12 01:32:19 christos Exp $	*/

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
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
 *	@(#)uipc_mbuf.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_mbuf.c,v 1.115 2006/10/12 01:32:19 christos Exp $");

#include "opt_mbuftrace.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#define MBTYPES
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/pool.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>

#include <uvm/uvm.h>


struct	pool mbpool;		/* mbuf pool */
struct	pool mclpool;		/* mbuf cluster pool */

struct pool_cache mbpool_cache;
struct pool_cache mclpool_cache;

struct mbstat mbstat;
int	max_linkhdr;
int	max_protohdr;
int	max_hdr;
int	max_datalen;

static int mb_ctor(void *, void *, int);

static void	*mclpool_alloc(struct pool *, int);
static void	mclpool_release(struct pool *, void *);

static struct pool_allocator mclpool_allocator = {
	.pa_alloc = mclpool_alloc,
	.pa_free = mclpool_release,
};

static struct mbuf *m_copym0(struct mbuf *, int, int, int, int);
static struct mbuf *m_split0(struct mbuf *, int, int, int);
static int m_copyback0(struct mbuf **, int, int, const void *, int, int);

/* flags for m_copyback0 */
#define	M_COPYBACK0_COPYBACK	0x0001	/* copyback from cp */
#define	M_COPYBACK0_PRESERVE	0x0002	/* preserve original data */
#define	M_COPYBACK0_COW		0x0004	/* do copy-on-write */
#define	M_COPYBACK0_EXTEND	0x0008	/* extend chain */

static const char mclpool_warnmsg[] =
    "WARNING: mclpool limit reached; increase NMBCLUSTERS";

MALLOC_DEFINE(M_MBUF, "mbuf", "mbuf");

#ifdef MBUFTRACE
struct mownerhead mowners = LIST_HEAD_INITIALIZER(mowners);
struct mowner unknown_mowners[] = {
	MOWNER_INIT("unknown", "free"),
	MOWNER_INIT("unknown", "data"),
	MOWNER_INIT("unknown", "header"),
	MOWNER_INIT("unknown", "soname"),
	MOWNER_INIT("unknown", "soopts"),
	MOWNER_INIT("unknown", "ftable"),
	MOWNER_INIT("unknown", "control"),
	MOWNER_INIT("unknown", "oobdata"),
};
struct mowner revoked_mowner = MOWNER_INIT("revoked", "");
#endif

/*
 * Initialize the mbuf allocator.
 */
void
mbinit(void)
{

	KASSERT(sizeof(struct _m_ext) <= MHLEN);
	KASSERT(sizeof(struct mbuf) == MSIZE);

	mclpool_allocator.pa_backingmap = mb_map;
	pool_init(&mbpool, msize, 0, 0, 0, "mbpl", NULL);
	pool_init(&mclpool, mclbytes, 0, 0, 0, "mclpl", &mclpool_allocator);

	pool_set_drain_hook(&mbpool, m_reclaim, NULL);
	pool_set_drain_hook(&mclpool, m_reclaim, NULL);

	pool_cache_init(&mbpool_cache, &mbpool, mb_ctor, NULL, NULL);
	pool_cache_init(&mclpool_cache, &mclpool, NULL, NULL, NULL);

	/*
	 * Set the hard limit on the mclpool to the number of
	 * mbuf clusters the kernel is to support.  Log the limit
	 * reached message max once a minute.
	 */
	pool_sethardlimit(&mclpool, nmbclusters, mclpool_warnmsg, 60);

	/*
	 * Set a low water mark for both mbufs and clusters.  This should
	 * help ensure that they can be allocated in a memory starvation
	 * situation.  This is important for e.g. diskless systems which
	 * must allocate mbufs in order for the pagedaemon to clean pages.
	 */
	pool_setlowat(&mbpool, mblowat);
	pool_setlowat(&mclpool, mcllowat);

#ifdef MBUFTRACE
	{
		/*
		 * Attach the unknown mowners.
		 */
		int i;
		MOWNER_ATTACH(&revoked_mowner);
		for (i = sizeof(unknown_mowners)/sizeof(unknown_mowners[0]);
		     i-- > 0; )
			MOWNER_ATTACH(&unknown_mowners[i]);
	}
#endif
}

/*
 * sysctl helper routine for the kern.mbuf subtree.  nmbclusters may
 * or may not be writable, and mblowat and mcllowat need range
 * checking and pool tweaking after being reset.
 */
static int
sysctl_kern_mbuf(SYSCTLFN_ARGS)
{
	int error, newval;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &newval;
	switch (rnode->sysctl_num) {
	case MBUF_NMBCLUSTERS:
		if (mb_map != NULL) {
			node.sysctl_flags &= ~CTLFLAG_READWRITE;
			node.sysctl_flags |= CTLFLAG_READONLY;
		}
		/* FALLTHROUGH */
	case MBUF_MBLOWAT:
	case MBUF_MCLLOWAT:
		newval = *(int*)rnode->sysctl_data;
		break;
	default:
		return (EOPNOTSUPP);
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);
	if (newval < 0)
		return (EINVAL);

	switch (node.sysctl_num) {
	case MBUF_NMBCLUSTERS:
		if (newval < nmbclusters)
			return (EINVAL);
		nmbclusters = newval;
		pool_sethardlimit(&mclpool, nmbclusters, mclpool_warnmsg, 60);
		break;
	case MBUF_MBLOWAT:
		mblowat = newval;
		pool_setlowat(&mbpool, mblowat);
		break;
	case MBUF_MCLLOWAT:
		mcllowat = newval;
		pool_setlowat(&mclpool, mcllowat);
		break;
	}

	return (0);
}

#ifdef MBUFTRACE
static int
sysctl_kern_mbuf_mowners(SYSCTLFN_ARGS)
{
	struct mowner *mo;
	size_t len = 0;
	int error = 0;

	if (namelen != 0)
		return (EINVAL);
	if (newp != NULL)
		return (EPERM);

	LIST_FOREACH(mo, &mowners, mo_link) {
		if (oldp != NULL) {
			if (*oldlenp - len < sizeof(*mo)) {
				error = ENOMEM;
				break;
			}
			error = copyout(mo, (caddr_t) oldp + len,
					sizeof(*mo));
			if (error)
				break;
		}
		len += sizeof(*mo);
	}

	if (error == 0)
		*oldlenp = len;

	return (error);
}
#endif /* MBUFTRACE */

SYSCTL_SETUP(sysctl_kern_mbuf_setup, "sysctl kern.mbuf subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "mbuf",
		       SYSCTL_DESCR("mbuf control variables"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_MBUF, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "msize",
		       SYSCTL_DESCR("mbuf base size"),
		       NULL, msize, NULL, 0,
		       CTL_KERN, KERN_MBUF, MBUF_MSIZE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "mclbytes",
		       SYSCTL_DESCR("mbuf cluster size"),
		       NULL, mclbytes, NULL, 0,
		       CTL_KERN, KERN_MBUF, MBUF_MCLBYTES, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nmbclusters",
		       SYSCTL_DESCR("Limit on the number of mbuf clusters"),
		       sysctl_kern_mbuf, 0, &nmbclusters, 0,
		       CTL_KERN, KERN_MBUF, MBUF_NMBCLUSTERS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "mblowat",
		       SYSCTL_DESCR("mbuf low water mark"),
		       sysctl_kern_mbuf, 0, &mblowat, 0,
		       CTL_KERN, KERN_MBUF, MBUF_MBLOWAT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "mcllowat",
		       SYSCTL_DESCR("mbuf cluster low water mark"),
		       sysctl_kern_mbuf, 0, &mcllowat, 0,
		       CTL_KERN, KERN_MBUF, MBUF_MCLLOWAT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("mbuf allocation statistics"),
		       NULL, 0, &mbstat, sizeof(mbstat),
		       CTL_KERN, KERN_MBUF, MBUF_STATS, CTL_EOL);
#ifdef MBUFTRACE
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "mowners",
		       SYSCTL_DESCR("Information about mbuf owners"),
		       sysctl_kern_mbuf_mowners, 0, NULL, 0,
		       CTL_KERN, KERN_MBUF, MBUF_MOWNERS, CTL_EOL);
#endif /* MBUFTRACE */
}

static void *
mclpool_alloc(struct pool *pp __unused, int flags)
{
	boolean_t waitok = (flags & PR_WAITOK) ? TRUE : FALSE;

	return ((void *)uvm_km_alloc_poolpage(mb_map, waitok));
}

static void
mclpool_release(struct pool *pp __unused, void *v)
{

	uvm_km_free_poolpage(mb_map, (vaddr_t)v);
}

/*ARGSUSED*/
static int
mb_ctor(void *arg __unused, void *object, int flags __unused)
{
	struct mbuf *m = object;

#ifdef POOL_VTOPHYS
	m->m_paddr = POOL_VTOPHYS(m);
#else
	m->m_paddr = M_PADDR_INVALID;
#endif
	return (0);
}

void
m_reclaim(void *arg __unused, int flags __unused)
{
	struct domain *dp;
	const struct protosw *pr;
	struct ifnet *ifp;
	int s = splvm();

	DOMAIN_FOREACH(dp) {
		for (pr = dp->dom_protosw;
		     pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_drain)
				(*pr->pr_drain)();
	}
	IFNET_FOREACH(ifp) {
		if (ifp->if_drain)
			(*ifp->if_drain)(ifp);
	}
	splx(s);
	mbstat.m_drain++;
}

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(int nowait, int type)
{
	struct mbuf *m;

	MGET(m, nowait, type);
	return (m);
}

struct mbuf *
m_gethdr(int nowait, int type)
{
	struct mbuf *m;

	MGETHDR(m, nowait, type);
	return (m);
}

struct mbuf *
m_getclr(int nowait, int type)
{
	struct mbuf *m;

	MGET(m, nowait, type);
	if (m == 0)
		return (NULL);
	memset(mtod(m, caddr_t), 0, MLEN);
	return (m);
}

void
m_clget(struct mbuf *m, int nowait)
{

	MCLGET(m, nowait);
}

struct mbuf *
m_free(struct mbuf *m)
{
	struct mbuf *n;

	MFREE(m, n);
	return (n);
}

void
m_freem(struct mbuf *m)
{
	struct mbuf *n;

	if (m == NULL)
		return;
	do {
		MFREE(m, n);
		m = n;
	} while (m);
}

#ifdef MBUFTRACE
/*
 * Walk a chain of mbufs, claiming ownership of each mbuf in the chain.
 */
void
m_claimm(struct mbuf *m, struct mowner *mo)
{

	for (; m != NULL; m = m->m_next)
		MCLAIM(m, mo);
}
#endif

/*
 * Mbuffer utility routines.
 */

/*
 * Lesser-used path for M_PREPEND:
 * allocate new mbuf to prepend to chain,
 * copy junk along.
 */
struct mbuf *
m_prepend(struct mbuf *m, int len, int how)
{
	struct mbuf *mn;

	MGET(mn, how, m->m_type);
	if (mn == (struct mbuf *)NULL) {
		m_freem(m);
		return ((struct mbuf *)NULL);
	}
	if (m->m_flags & M_PKTHDR) {
		M_MOVE_PKTHDR(mn, m);
	} else {
		MCLAIM(mn, m->m_owner);
	}
	mn->m_next = m;
	m = mn;
	if (len < MHLEN)
		MH_ALIGN(m, len);
	m->m_len = len;
	return (m);
}

/*
 * Make a copy of an mbuf chain starting "off0" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * The wait parameter is a choice of M_WAIT/M_DONTWAIT from caller.
 */
int MCFail;

struct mbuf *
m_copym(struct mbuf *m, int off0, int len, int wait)
{

	return m_copym0(m, off0, len, wait, 0);	/* shallow copy on M_EXT */
}

struct mbuf *
m_dup(struct mbuf *m, int off0, int len, int wait)
{

	return m_copym0(m, off0, len, wait, 1);	/* deep copy */
}

static struct mbuf *
m_copym0(struct mbuf *m, int off0, int len, int wait, int deep)
{
	struct mbuf *n, **np;
	int off = off0;
	struct mbuf *top;
	int copyhdr = 0;

	if (off < 0 || len < 0)
		panic("m_copym: off %d, len %d", off, len);
	if (off == 0 && m->m_flags & M_PKTHDR)
		copyhdr = 1;
	while (off > 0) {
		if (m == 0)
			panic("m_copym: m == 0, off %d", off);
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copym: m == 0, len %d [!COPYALL]",
				    len);
			break;
		}
		MGET(n, wait, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		MCLAIM(n, m->m_owner);
		if (copyhdr) {
			M_COPY_PKTHDR(n, m);
			if (len == M_COPYALL)
				n->m_pkthdr.len -= off0;
			else
				n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		n->m_len = min(len, m->m_len - off);
		if (m->m_flags & M_EXT) {
			if (!deep) {
				n->m_data = m->m_data + off;
				n->m_ext = m->m_ext;
				MCLADDREFERENCE(m, n);
			} else {
				/*
				 * we are unsure about the way m was allocated.
				 * copy into multiple MCLBYTES cluster mbufs.
				 */
				MCLGET(n, wait);
				n->m_len = 0;
				n->m_len = M_TRAILINGSPACE(n);
				n->m_len = min(n->m_len, len);
				n->m_len = min(n->m_len, m->m_len - off);
				memcpy(mtod(n, caddr_t), mtod(m, caddr_t) + off,
				    (unsigned)n->m_len);
			}
		} else
			memcpy(mtod(n, caddr_t), mtod(m, caddr_t)+off,
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off += n->m_len;
#ifdef DIAGNOSTIC
		if (off > m->m_len)
			panic("m_copym0 overrun");
#endif
		if (off == m->m_len) {
			m = m->m_next;
			off = 0;
		}
		np = &n->m_next;
	}
	if (top == 0)
		MCFail++;
	return (top);
nospace:
	m_freem(top);
	MCFail++;
	return (NULL);
}

/*
 * Copy an entire packet, including header (which must be present).
 * An optimization of the common case `m_copym(m, 0, M_COPYALL, how)'.
 */
struct mbuf *
m_copypacket(struct mbuf *m, int how)
{
	struct mbuf *top, *n, *o;

	MGET(n, how, m->m_type);
	top = n;
	if (!n)
		goto nospace;

	MCLAIM(n, m->m_owner);
	M_COPY_PKTHDR(n, m);
	n->m_len = m->m_len;
	if (m->m_flags & M_EXT) {
		n->m_data = m->m_data;
		n->m_ext = m->m_ext;
		MCLADDREFERENCE(m, n);
	} else {
		memcpy(mtod(n, char *), mtod(m, char *), n->m_len);
	}

	m = m->m_next;
	while (m) {
		MGET(o, how, m->m_type);
		if (!o)
			goto nospace;

		MCLAIM(o, m->m_owner);
		n->m_next = o;
		n = n->m_next;

		n->m_len = m->m_len;
		if (m->m_flags & M_EXT) {
			n->m_data = m->m_data;
			n->m_ext = m->m_ext;
			MCLADDREFERENCE(m, n);
		} else {
			memcpy(mtod(n, char *), mtod(m, char *), n->m_len);
		}

		m = m->m_next;
	}
	return top;
nospace:
	m_freem(top);
	MCFail++;
	return NULL;
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void
m_copydata(struct mbuf *m, int off, int len, void *vp)
{
	unsigned	count;
	caddr_t		cp = vp;

	if (off < 0 || len < 0)
		panic("m_copydata: off %d, len %d", off, len);
	while (off > 0) {
		if (m == NULL)
			panic("m_copydata: m == NULL, off %d", off);
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == NULL)
			panic("m_copydata: m == NULL, len %d", len);
		count = min(m->m_len - off, len);
		memcpy(cp, mtod(m, caddr_t) + off, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

/*
 * Concatenate mbuf chain n to m.
 * n might be copied into m (when n->m_len is small), therefore data portion of
 * n could be copied into an mbuf of different mbuf type.
 * Any m_pkthdr is not updated.
 */
void
m_cat(struct mbuf *m, struct mbuf *n)
{

	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (M_READONLY(m) || n->m_len > M_TRAILINGSPACE(m)) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		memcpy(mtod(m, caddr_t) + m->m_len, mtod(n, caddr_t),
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

void
m_adj(struct mbuf *mp, int req_len)
{
	int len = req_len;
	struct mbuf *m;
	int count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		/*
		 * Trim from head.
		 */
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_data += len;
				len = 0;
			}
		}
		m = mp;
		if (mp->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= (req_len - len);
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			if (mp->m_flags & M_PKTHDR)
				mp->m_pkthdr.len -= len;
			return;
		}
		count -= len;
		if (count < 0)
			count = 0;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		m = mp;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len = count;
		for (; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		if (m)
			while (m->m_next)
				(m = m->m_next)->m_len = 0;
	}
}

/*
 * Rearrange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to max_protohdr-len extra bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
int MPFail;

struct mbuf *
m_pullup(struct mbuf *n, int len)
{
	struct mbuf *m;
	int count;
	int space;

	/*
	 * If first mbuf has no cluster, and has room for len bytes
	 * without shifting current data, pullup into it,
	 * otherwise allocate a new mbuf to prepend to the chain.
	 */
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data + len < &n->m_dat[MLEN] && n->m_next) {
		if (n->m_len >= len)
			return (n);
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MHLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		MCLAIM(m, n->m_owner);
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR) {
			M_MOVE_PKTHDR(m, n);
		}
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		memcpy(mtod(m, caddr_t) + m->m_len, mtod(n, caddr_t),
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	MPFail++;
	return (NULL);
}

/*
 * Like m_pullup(), except a new mbuf is always allocated, and we allow
 * the amount of empty space before the data in the new mbuf to be specified
 * (in the event that the caller expects to prepend later).
 */
int MSFail;

struct mbuf *
m_copyup(struct mbuf *n, int len, int dstoff)
{
	struct mbuf *m;
	int count, space;

	if (len > (MHLEN - dstoff))
		goto bad;
	MGET(m, M_DONTWAIT, n->m_type);
	if (m == NULL)
		goto bad;
	MCLAIM(m, n->m_owner);
	m->m_len = 0;
	if (n->m_flags & M_PKTHDR) {
		M_MOVE_PKTHDR(m, n);
	}
	m->m_data += dstoff;
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		memcpy(mtod(m, caddr_t) + m->m_len, mtod(n, caddr_t),
		    (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
 bad:
	m_freem(n);
	MSFail++;
	return (NULL);
}

/*
 * Partition an mbuf chain in two pieces, returning the tail --
 * all but the first len0 bytes.  In case of failure, it returns NULL and
 * attempts to restore the chain to its original state.
 */
struct mbuf *
m_split(struct mbuf *m0, int len0, int wait)
{

	return m_split0(m0, len0, wait, 1);
}

static struct mbuf *
m_split0(struct mbuf *m0, int len0, int wait, int copyhdr)
{
	struct mbuf *m, *n;
	unsigned len = len0, remain, len_save;

	for (m = m0; m && len > m->m_len; m = m->m_next)
		len -= m->m_len;
	if (m == 0)
		return (NULL);
	remain = m->m_len - len;
	if (copyhdr && (m0->m_flags & M_PKTHDR)) {
		MGETHDR(n, wait, m0->m_type);
		if (n == 0)
			return (NULL);
		MCLAIM(n, m0->m_owner);
		n->m_pkthdr.rcvif = m0->m_pkthdr.rcvif;
		n->m_pkthdr.len = m0->m_pkthdr.len - len0;
		len_save = m0->m_pkthdr.len;
		m0->m_pkthdr.len = len0;
		if (m->m_flags & M_EXT)
			goto extpacket;
		if (remain > MHLEN) {
			/* m can't be the lead packet */
			MH_ALIGN(n, 0);
			n->m_next = m_split(m, len, wait);
			if (n->m_next == 0) {
				(void) m_free(n);
				m0->m_pkthdr.len = len_save;
				return (NULL);
			} else
				return (n);
		} else
			MH_ALIGN(n, remain);
	} else if (remain == 0) {
		n = m->m_next;
		m->m_next = 0;
		return (n);
	} else {
		MGET(n, wait, m->m_type);
		if (n == 0)
			return (NULL);
		MCLAIM(n, m->m_owner);
		M_ALIGN(n, remain);
	}
extpacket:
	if (m->m_flags & M_EXT) {
		n->m_ext = m->m_ext;
		MCLADDREFERENCE(m, n);
		n->m_data = m->m_data + len;
	} else {
		memcpy(mtod(n, caddr_t), mtod(m, caddr_t) + len, remain);
	}
	n->m_len = remain;
	m->m_len = len;
	n->m_next = m->m_next;
	m->m_next = 0;
	return (n);
}
/*
 * Routine to copy from device local memory into mbufs.
 */
struct mbuf *
m_devget(char *buf, int totlen, int off0, struct ifnet *ifp,
    void (*copy)(const void *from, void *to, size_t len))
{
	struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	int off = off0, len;
	char *cp;
	char *epkt;

	cp = buf;
	epkt = cp + totlen;
	if (off) {
		/*
		 * If 'off' is non-zero, packet is trailer-encapsulated,
		 * so we have to skip the type and length fields.
		 */
		cp += off + 2 * sizeof(uint16_t);
		totlen -= 2 * sizeof(uint16_t);
	}
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (NULL);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (NULL);
			}
			m->m_len = MLEN;
		}
		len = min(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_free(m);
				m_freem(top);
				return (NULL);
			}
			m->m_len = len = min(len, MCLBYTES);
		} else {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		if (copy)
			copy(cp, mtod(m, caddr_t), (size_t)len);
		else
			memcpy(mtod(m, caddr_t), cp, (size_t)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return (top);
}

/*
 * Copy data from a buffer back into the indicated mbuf chain,
 * starting "off" bytes from the beginning, extending the mbuf
 * chain if necessary.
 */
void
m_copyback(struct mbuf *m0, int off, int len, const void *cp)
{
#if defined(DEBUG)
	struct mbuf *origm = m0;
	int error;
#endif /* defined(DEBUG) */

	if (m0 == NULL)
		return;

#if defined(DEBUG)
	error =
#endif /* defined(DEBUG) */
	m_copyback0(&m0, off, len, cp,
	    M_COPYBACK0_COPYBACK|M_COPYBACK0_EXTEND, M_DONTWAIT);

#if defined(DEBUG)
	if (error != 0 || (m0 != NULL && origm != m0))
		panic("m_copyback");
#endif /* defined(DEBUG) */
}

struct mbuf *
m_copyback_cow(struct mbuf *m0, int off, int len, const void *cp, int how)
{
	int error;

	/* don't support chain expansion */
	KDASSERT(off + len <= m_length(m0));

	error = m_copyback0(&m0, off, len, cp,
	    M_COPYBACK0_COPYBACK|M_COPYBACK0_COW, how);
	if (error) {
		/*
		 * no way to recover from partial success.
		 * just free the chain.
		 */
		m_freem(m0);
		return NULL;
	}
	return m0;
}

/*
 * m_makewritable: ensure the specified range writable.
 */
int
m_makewritable(struct mbuf **mp, int off, int len, int how)
{
	int error;
#if defined(DEBUG)
	struct mbuf *n;
	int origlen, reslen;

	origlen = m_length(*mp);
#endif /* defined(DEBUG) */

#if 0 /* M_COPYALL is large enough */
	if (len == M_COPYALL)
		len = m_length(*mp) - off; /* XXX */
#endif

	error = m_copyback0(mp, off, len, NULL,
	    M_COPYBACK0_PRESERVE|M_COPYBACK0_COW, how);

#if defined(DEBUG)
	reslen = 0;
	for (n = *mp; n; n = n->m_next)
		reslen += n->m_len;
	if (origlen != reslen)
		panic("m_makewritable: length changed");
	if (((*mp)->m_flags & M_PKTHDR) != 0 && reslen != (*mp)->m_pkthdr.len)
		panic("m_makewritable: inconsist");
#endif /* defined(DEBUG) */

	return error;
}

int
m_copyback0(struct mbuf **mp0, int off, int len, const void *vp, int flags,
    int how)
{
	int mlen;
	struct mbuf *m, *n;
	struct mbuf **mp;
	int totlen = 0;
	const char *cp = vp;

	KASSERT(mp0 != NULL);
	KASSERT(*mp0 != NULL);
	KASSERT((flags & M_COPYBACK0_PRESERVE) == 0 || cp == NULL);
	KASSERT((flags & M_COPYBACK0_COPYBACK) == 0 || cp != NULL);

	/*
	 * we don't bother to update "totlen" in the case of M_COPYBACK0_COW,
	 * assuming that M_COPYBACK0_EXTEND and M_COPYBACK0_COW are exclusive.
	 */

	KASSERT((~flags & (M_COPYBACK0_EXTEND|M_COPYBACK0_COW)) != 0);

	mp = mp0;
	m = *mp;
	while (off > (mlen = m->m_len)) {
		off -= mlen;
		totlen += mlen;
		if (m->m_next == NULL) {
			int tspace;
extend:
			if ((flags & M_COPYBACK0_EXTEND) == 0)
				goto out;

			/*
			 * try to make some space at the end of "m".
			 */

			mlen = m->m_len;
			if (off + len >= MINCLSIZE &&
			    (m->m_flags & M_EXT) == 0 && m->m_len == 0) {
				MCLGET(m, how);
			}
			tspace = M_TRAILINGSPACE(m);
			if (tspace > 0) {
				tspace = min(tspace, off + len);
				KASSERT(tspace > 0);
				memset(mtod(m, char *) + m->m_len, 0,
				    min(off, tspace));
				m->m_len += tspace;
				off += mlen;
				totlen -= mlen;
				continue;
			}

			/*
			 * need to allocate an mbuf.
			 */

			if (off + len >= MINCLSIZE) {
				n = m_getcl(how, m->m_type, 0);
			} else {
				n = m_get(how, m->m_type);
			}
			if (n == NULL) {
				goto out;
			}
			n->m_len = 0;
			n->m_len = min(M_TRAILINGSPACE(n), off + len);
			memset(mtod(n, char *), 0, min(n->m_len, off));
			m->m_next = n;
		}
		mp = &m->m_next;
		m = m->m_next;
	}
	while (len > 0) {
		mlen = m->m_len - off;
		if (mlen != 0 && M_READONLY(m)) {
			char *datap;
			int eatlen;

			/*
			 * this mbuf is read-only.
			 * allocate a new writable mbuf and try again.
			 */

#if defined(DIAGNOSTIC)
			if ((flags & M_COPYBACK0_COW) == 0)
				panic("m_copyback0: read-only");
#endif /* defined(DIAGNOSTIC) */

			/*
			 * if we're going to write into the middle of
			 * a mbuf, split it first.
			 */
			if (off > 0 && len < mlen) {
				n = m_split0(m, off, how, 0);
				if (n == NULL)
					goto enobufs;
				m->m_next = n;
				mp = &m->m_next;
				m = n;
				off = 0;
				continue;
			}

			/*
			 * XXX TODO coalesce into the trailingspace of
			 * the previous mbuf when possible.
			 */

			/*
			 * allocate a new mbuf.  copy packet header if needed.
			 */
			MGET(n, how, m->m_type);
			if (n == NULL)
				goto enobufs;
			MCLAIM(n, m->m_owner);
			if (off == 0 && (m->m_flags & M_PKTHDR) != 0) {
				M_MOVE_PKTHDR(n, m);
				n->m_len = MHLEN;
			} else {
				if (len >= MINCLSIZE)
					MCLGET(n, M_DONTWAIT);
				n->m_len =
				    (n->m_flags & M_EXT) ? MCLBYTES : MLEN;
			}
			if (n->m_len > len)
				n->m_len = len;

			/*
			 * free the region which has been overwritten.
			 * copying data from old mbufs if requested.
			 */
			if (flags & M_COPYBACK0_PRESERVE)
				datap = mtod(n, char *);
			else
				datap = NULL;
			eatlen = n->m_len;
			KDASSERT(off == 0 || eatlen >= mlen);
			if (off > 0) {
				KDASSERT(len >= mlen);
				m->m_len = off;
				m->m_next = n;
				if (datap) {
					m_copydata(m, off, mlen, datap);
					datap += mlen;
				}
				eatlen -= mlen;
				mp = &m->m_next;
				m = m->m_next;
			}
			while (m != NULL && M_READONLY(m) &&
			    n->m_type == m->m_type && eatlen > 0) {
				mlen = min(eatlen, m->m_len);
				if (datap) {
					m_copydata(m, 0, mlen, datap);
					datap += mlen;
				}
				m->m_data += mlen;
				m->m_len -= mlen;
				eatlen -= mlen;
				if (m->m_len == 0)
					*mp = m = m_free(m);
			}
			if (eatlen > 0)
				n->m_len -= eatlen;
			n->m_next = m;
			*mp = m = n;
			continue;
		}
		mlen = min(mlen, len);
		if (flags & M_COPYBACK0_COPYBACK) {
			memcpy(mtod(m, caddr_t) + off, cp, (unsigned)mlen);
			cp += mlen;
		}
		len -= mlen;
		mlen += off;
		off = 0;
		totlen += mlen;
		if (len == 0)
			break;
		if (m->m_next == NULL) {
			goto extend;
		}
		mp = &m->m_next;
		m = m->m_next;
	}
out:	if (((m = *mp0)->m_flags & M_PKTHDR) && (m->m_pkthdr.len < totlen)) {
		KASSERT((flags & M_COPYBACK0_EXTEND) != 0);
		m->m_pkthdr.len = totlen;
	}

	return 0;

enobufs:
	return ENOBUFS;
}

void
m_move_pkthdr(struct mbuf *to, struct mbuf *from)
{

	KASSERT((to->m_flags & M_EXT) == 0);
	KASSERT((to->m_flags & M_PKTHDR) == 0 || m_tag_first(to) == NULL);
	KASSERT((from->m_flags & M_PKTHDR) != 0);

	to->m_pkthdr = from->m_pkthdr;
	to->m_flags = from->m_flags & M_COPYFLAGS;
	to->m_data = to->m_pktdat;

	from->m_flags &= ~M_PKTHDR;
}

/*
 * Apply function f to the data in an mbuf chain starting "off" bytes from the
 * beginning, continuing for "len" bytes.
 */
int
m_apply(struct mbuf *m, int off, int len,
    int (*f)(void *, caddr_t, unsigned int), void *arg)
{
	unsigned int count;
	int rval;

	KASSERT(len >= 0);
	KASSERT(off >= 0);

	while (off > 0) {
		KASSERT(m != NULL);
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		KASSERT(m != NULL);
		count = min(m->m_len - off, len);

		rval = (*f)(arg, mtod(m, caddr_t) + off, count);
		if (rval)
			return (rval);

		len -= count;
		off = 0;
		m = m->m_next;
	}

	return (0);
}

/*
 * Return a pointer to mbuf/offset of location in mbuf chain.
 */
struct mbuf *
m_getptr(struct mbuf *m, int loc, int *off)
{

	while (loc >= 0) {
		/* Normal end of search */
		if (m->m_len > loc) {
	    		*off = loc;
	    		return (m);
		} else {
	    		loc -= m->m_len;

	    		if (m->m_next == NULL) {
				if (loc == 0) {
 					/* Point at the end of valid data */
		    			*off = m->m_len;
		    			return (m);
				} else
		  			return (NULL);
	    		} else
	      			m = m->m_next;
		}
    	}

	return (NULL);
}

#if defined(DDB)
void
m_print(const struct mbuf *m, const char *modif, void (*pr)(const char *, ...))
{
	char ch;
	boolean_t opt_c = FALSE;
	char buf[512];

	while ((ch = *(modif++)) != '\0') {
		switch (ch) {
		case 'c':
			opt_c = TRUE;
			break;
		}
	}

nextchain:
	(*pr)("MBUF %p\n", m);
	bitmask_snprintf(m->m_flags, M_FLAGS_BITS, buf, sizeof(buf));
	(*pr)("  data=%p, len=%d, type=%d, flags=0x%s\n",
	    m->m_data, m->m_len, m->m_type, buf);
	(*pr)("  owner=%p, next=%p, nextpkt=%p\n", m->m_owner, m->m_next,
	    m->m_nextpkt);
	(*pr)("  leadingspace=%u, trailingspace=%u, readonly=%u\n",
	    (int)M_LEADINGSPACE(m), (int)M_TRAILINGSPACE(m),
	    (int)M_READONLY(m));
	if ((m->m_flags & M_PKTHDR) != 0) {
		bitmask_snprintf(m->m_pkthdr.csum_flags, M_CSUM_BITS, buf,
		    sizeof(buf));
		(*pr)("  pktlen=%d, rcvif=%p, csum_flags=0x%s, csum_data=0x%"
		    PRIx32 ", segsz=%u\n",
		    m->m_pkthdr.len, m->m_pkthdr.rcvif,
		    buf, m->m_pkthdr.csum_data, m->m_pkthdr.segsz);
	}
	if ((m->m_flags & M_EXT)) {
		(*pr)("  shared=%u, ext_buf=%p, ext_size=%zd, "
		    "ext_free=%p, ext_arg=%p\n",
		    (int)MCLISREFERENCED(m),
		    m->m_ext.ext_buf, m->m_ext.ext_size,
		    m->m_ext.ext_free, m->m_ext.ext_arg);
	}
	if ((~m->m_flags & (M_EXT|M_EXT_PAGES)) == 0) {
		vaddr_t sva = (vaddr_t)m->m_ext.ext_buf;
		vaddr_t eva = sva + m->m_ext.ext_size;
		int n = (round_page(eva) - trunc_page(sva)) >> PAGE_SHIFT;
		int i;

		(*pr)("  pages:");
		for (i = 0; i < n; i ++) {
			(*pr)(" %p", m->m_ext.ext_pgs[i]);
		}
		(*pr)("\n");
	}

	if (opt_c) {
		m = m->m_next;
		if (m != NULL) {
			goto nextchain;
		}
	}
}
#endif /* defined(DDB) */
