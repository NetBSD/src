.\"	$Id: 9.t,v 1.1 2001/07/04 05:29:25 itojun Exp $
.\"
.\".ds RH Related work
.NH 1
Related work
.PP
Van Jacobson proposed pbuf structure \**
.FS
A reference should be here,
but I'm having hard time finding published literature for it.
.FE
as an alternative to BSD mbuf structure.
The proposal has two main arguments.
First is the use of continuous data buffer, instead of chained fragments
like mbufs.
Another is the improvement to TCP performance by restructuring
TCP input/output handling.
While the latter point still holds for IPv6,
we believe that the former point must be reviewed carefully before being used with IPv6.
Our experience suggests that we need to insert many intermediate extension headers into
the packet data during IPv6 outbound packet processing.
We believe that mbuf is more suitable
than the proposed pbuf structure for handling the packet data efficiently.
Using pbuf may result in the making of more copies than in the mbuf case.
.PP
In a cross-BSD portability paper,
.[
metz four bsds
.]
Craig Metz described
.I nbuf
structure in NRL IPv6/IPsec stack.
nbuf is a wrapper structure used to unify linux linear-buffer packet management
and BSD mbuf structure, and is not closely related to the topic of this paper.
The
.I m_pullup2
example discussed in this paper is drawn from the NRL implementation.
.\".ds RH Conclusions
.NH 1
Conclusions
.PP
This paper discussed mbuf manipulation in a 4.4BSD-based IPv6/IPsec stack,
namely KAME IPv6/IPsec implementation.
4.4BSD makes certain assumptions regarding packet header length and its format.
For IPv6/IPsec support, we removed those assumptions from the
4.4BSD code.
We introduced the
.I m_pulldown
function and a new function call sequence for inbound packet processing.
These innovations helped us to implement IPv6/IPsec in a very spec-conformant manner,
with fewer implementation restrictions added against specifications.
.PP
The described code is publically available, under a BSD-like license,
at \f[CR]ftp://ftp.kame.net/\fP.
KAME IPv6/IPsec stack is being merged into 4.4BSD variants like FreeBSD,
NetBSD and OpenBSD.
An integration into BSD/OS is planned.
We will be able to see official releases of these OSes with KAME code soon.
.PP
.\".ds RH Acknowledgements
.NH 1
Acknowledgements
.PP
The paper was made possible by the collective efforts of researchers at
the KAME project and the WIDE project and of other IPv6 implementers at large.
We would also like to acknowledge all four BSD groups who helped
us improve the KAME IPv6 stack code
by sending bug reports and improvement suggestions,
and the Freenix reviewers helped polish the paper.
.[
$LIST$
.]
.if t .2C
.LD
.ps 5
.vs 6
\f[CR]\s5/*
 * ensure that [off, off + len) is contiguous on the mbuf chain "m".
 * packet chain before "off" is kept untouched.
 * if offp == NULL, the target will start at <retval, 0> on resulting chain.
 * if offp != NULL, the target will start at <retval, *offp> on resulting chain.
 *
 * on error return (NULL return value), original "m" will be freed.
 *
 * XXX M_TRAILINGSPACE/M_LEADINGSPACE on shared cluster (sharedcluster)
 */
struct mbuf *
m_pulldown(m, off, len, offp)
	struct mbuf *m;
	int off, len;
	int *offp;
{
	struct mbuf *n, *o;
	int hlen, tlen, olen;
	int sharedcluster;

	/* check invalid arguments. */
	if (m == NULL)
		panic("m == NULL in m_pulldown()");
	if (len > MCLBYTES) {
		m_freem(m);
		return NULL;	/* impossible */
	}

	n = m;
	while (n != NULL && off > 0) {
		if (n->m_len > off)
			break;
		off -= n->m_len;
		n = n->m_next;
	}
	/* be sure to point non-empty mbuf */
	while (n != NULL && n->m_len == 0)
		n = n->m_next;
	if (!n) {
		m_freem(m);
		return NULL;	/* mbuf chain too short */
	}

	/*
	 * the target data is on <n, off>.
	 * if we got enough data on the mbuf "n", we're done.
	 */
	if ((off == 0 || offp) && len <= n->m_len - off)
		goto ok;

	/*
	 * when len < n->m_len - off and off != 0, it is a special case.
	 * len bytes from <n, off> sits in single mbuf, but the caller does
	 * not like the starting position (off).
	 * chop the current mbuf into two pieces, set off to 0.
	 */
	if (len < n->m_len - off) {
		o = m_copym(n, off, n->m_len - off, M_DONTWAIT);
		if (o == NULL) {
			m_freem(m);
			return NULL;	/* ENOBUFS */
		}
		n->m_len = off;
		o->m_next = n->m_next;
		n->m_next = o;
		n = n->m_next;
		off = 0;
		goto ok;
	}

	/*
	 * we need to take hlen from <n, off> and tlen from <n->m_next, 0>,
	 * and construct contiguous mbuf with m_len == len.
	 * note that hlen + tlen == len, and tlen > 0.
	 */
	hlen = n->m_len - off;
	tlen = len - hlen;

	/*
	 * ensure that we have enough trailing data on mbuf chain.
	 * if not, we can do nothing about the chain.
	 */
	olen = 0;
	for (o = n->m_next; o != NULL; o = o->m_next)
		olen += o->m_len;
	if (hlen + olen < len) {
		m_freem(m);
		return NULL;	/* mbuf chain too short */
	}

	/*
	 * easy cases first.
	 * we need to use m_copydata() to get data from <n->m_next, 0>.
	 */
	if ((n->m_flags & M_EXT) == 0)
		sharedcluster = 0;
	else {
		if (n->m_ext.ext_free)
			sharedcluster = 1;
		else if (MCLISREFERENCED(n))
			sharedcluster = 1;
		else
			sharedcluster = 0;
	}
	if ((off == 0 || offp) && M_TRAILINGSPACE(n) >= tlen
	 && !sharedcluster) {
		m_copydata(n->m_next, 0, tlen, mtod(n, caddr_t) + n->m_len);
		n->m_len += tlen;
		m_adj(n->m_next, tlen);
		goto ok;
	}
	if ((off == 0 || offp) && M_LEADINGSPACE(n->m_next) >= hlen
	 && !sharedcluster) {
		n->m_next->m_data -= hlen;
		n->m_next->m_len += hlen;
		bcopy(mtod(n, caddr_t) + off, mtod(n->m_next, caddr_t), hlen);
		n->m_len -= hlen;
		n = n->m_next;
		off = 0;
		goto ok;
	}

	/*
	 * now, we need to do the hard way.  don't m_copy as there's no room
	 * on both end.
	 */
	MGET(o, M_DONTWAIT, m->m_type);
	if (o == NULL) {
		m_freem(m);
		return NULL;	/* ENOBUFS */
	}
	if (len > MHLEN) {	/* use MHLEN just for safety */
		MCLGET(o, M_DONTWAIT);
		if ((o->m_flags & M_EXT) == 0) {
			m_freem(m);
			m_free(o);
			return NULL;	/* ENOBUFS */
		}
	}
	/* get hlen from <n, off> into <o, 0> */
	o->m_len = hlen;
	bcopy(mtod(n, caddr_t) + off, mtod(o, caddr_t), hlen);
	n->m_len -= hlen;
	/* get tlen from <n->m_next, 0> into <o, hlen> */
	m_copydata(n->m_next, 0, tlen, mtod(o, caddr_t) + o->m_len);
	o->m_len += tlen;
	m_adj(n->m_next, tlen);
	o->m_next = n->m_next;
	n->m_next = o;
	n = o;
	off = 0;

ok:
	if (offp)
		*offp = off;
	return n;
}
.DE
