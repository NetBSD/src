/*	$NetBSD: ipsec_mbuf.c,v 1.21.2.4 2018/06/25 07:26:07 pgoyette Exp $	*/

/*
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: sys/netipsec/ipsec_mbuf.c,v 1.5.2.2 2003/03/28 20:32:53 sam Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipsec_mbuf.c,v 1.21.2.4 2018/06/25 07:26:07 pgoyette Exp $");

/*
 * IPsec-specific mbuf routines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>

/*
 * Create a writable copy of the mbuf chain.  While doing this
 * we compact the chain with a goal of producing a chain with
 * at most two mbufs.  The second mbuf in this chain is likely
 * to be a cluster.  The primary purpose of this work is to create
 * a writable packet for encryption, compression, etc.  The
 * secondary goal is to linearize the data so the data can be
 * passed to crypto hardware in the most efficient manner possible.
 */
struct mbuf *
m_clone(struct mbuf *m0)
{
	struct mbuf *m, *mprev;
	struct mbuf *n, *mfirst, *mlast;
	int len, off;

	KASSERT(m0 != NULL);

	mprev = NULL;
	for (m = m0; m != NULL; m = mprev->m_next) {
		/*
		 * Regular mbufs are ignored unless there's a cluster
		 * in front of it that we can use to coalesce.
		 */
		if ((m->m_flags & M_EXT) == 0) {
			if (mprev && (mprev->m_flags & M_EXT) &&
			    m->m_len <= M_TRAILINGSPACE(mprev)) {
				memcpy(mtod(mprev, char *) + mprev->m_len,
				    mtod(m, char *), m->m_len);
				mprev->m_len += m->m_len;
				mprev->m_next = m_free(m);
				IPSEC_STATINC(IPSEC_STAT_MBCOALESCED);
			} else {
				mprev = m;
			}
			continue;
		}

		/*
		 * Writable mbufs are left alone.
		 */
		if (!M_READONLY(m)) {
			mprev = m;
			continue;
		}

		/*
		 * Not writable, replace with a copy or coalesce with
		 * the previous mbuf if possible (since we have to copy
		 * it anyway, we try to reduce the number of mbufs and
		 * clusters so that future work is easier).
		 */

		/* We only coalesce into a cluster. */
		if (mprev != NULL && (mprev->m_flags & M_EXT) &&
		    m->m_len <= M_TRAILINGSPACE(mprev)) {
			memcpy(mtod(mprev, char *) + mprev->m_len,
			    mtod(m, char *), m->m_len);
			mprev->m_len += m->m_len;
			mprev->m_next = m_free(m);
			IPSEC_STATINC(IPSEC_STAT_CLCOALESCED);
			continue;
		}

		/*
		 * Allocate new space to hold the copy...
		 */
		if (mprev == NULL && (m->m_flags & M_PKTHDR)) {
			MGETHDR(n, M_DONTWAIT, m->m_type);
			if (n == NULL) {
				m_freem(m0);
				return NULL;
			}
			M_MOVE_PKTHDR(n, m);
			MCLGET(n, M_DONTWAIT);
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				m_freem(m0);
				return NULL;
			}
		} else {
			n = m_getcl(M_DONTWAIT, m->m_type, m->m_flags);
			if (n == NULL) {
				m_freem(m0);
				return NULL;
			}
		}

		/*
		 * ... and copy the data.  We deal with jumbo mbufs
		 * (i.e. m_len > MCLBYTES) by splitting them into
		 * clusters.  We could just malloc a buffer and make
		 * it external but too many device drivers don't know
		 * how to break up the non-contiguous memory when
		 * doing DMA.
		 */
		len = m->m_len;
		off = 0;
		mfirst = n;
		mlast = NULL;
		for (;;) {
			const int cc = min(len, MCLBYTES);
			memcpy(mtod(n, char *), mtod(m, char *) + off, cc);
			n->m_len = cc;
			if (mlast != NULL)
				mlast->m_next = n;
			mlast = n;
			IPSEC_STATINC(IPSEC_STAT_CLCOPIED);

			len -= cc;
			if (len <= 0)
				break;
			off += cc;

			n = m_getcl(M_DONTWAIT, m->m_type, m->m_flags);
			if (n == NULL) {
				m_freem(mfirst);
				m_freem(m0);
				return NULL;
			}
		}
		n->m_next = m->m_next;
		if (mprev == NULL)
			m0 = mfirst;		/* new head of chain */
		else
			mprev->m_next = mfirst;	/* replace old mbuf */
		m_free(m);			/* release old mbuf */
		mprev = mfirst;
	}

	return m0;
}

/*
 * Make space for a new header of length hlen at skip bytes
 * into the packet.  When doing this we allocate new mbufs only
 * when absolutely necessary.  The mbuf where the new header
 * is to go is returned together with an offset into the mbuf.
 * If NULL is returned then the mbuf chain may have been modified;
 * the caller is assumed to always free the chain.
 */
struct mbuf *
m_makespace(struct mbuf *m0, int skip, int hlen, int *off)
{
	struct mbuf *m;
	unsigned remain;

	KASSERT(m0 != NULL);
	KASSERT(m0->m_flags & M_PKTHDR);
	KASSERTMSG(hlen < MHLEN, "hlen too big: %u", hlen);

	for (m = m0; m && skip > m->m_len; m = m->m_next)
		skip -= m->m_len;
	if (m == NULL)
		return NULL;

	/*
	 * At this point skip is the offset into the mbuf m
	 * where the new header should be placed.  Figure out
	 * if there's space to insert the new header.  If so,
	 * and copying the remainder makes sense then do so.
	 * Otherwise insert a new mbuf in the chain, splitting
	 * the contents of m as needed.
	 */
	remain = m->m_len - skip;		/* data to move */
	if (hlen > M_TRAILINGSPACE(m)) {
		struct mbuf *n0, *n, **np;
		int todo, len, done, alloc;

		n0 = NULL;
		np = &n0;
		alloc = 0;
		done = 0;
		todo = remain;
		while (todo > 0) {
			if (todo > MHLEN) {
				n = m_getcl(M_DONTWAIT, m->m_type, 0);
				len = MCLBYTES;
			} else {
				n = m_get(M_DONTWAIT, m->m_type);
				len = MHLEN;
			}
			if (n == NULL) {
				m_freem(n0);
				return NULL;
			}
			*np = n;
			np = &n->m_next;
			alloc++;
			len = min(todo, len);
			memcpy(n->m_data, mtod(m, char *) + skip + done, len);
			n->m_len = len;
			done += len;
			todo -= len;
		}

		if (hlen <= M_TRAILINGSPACE(m) + remain) {
			m->m_len = skip + hlen;
			*off = skip;
			if (n0 != NULL) {
				*np = m->m_next;
				m->m_next = n0;
			}
		} else {
			n = m_get(M_DONTWAIT, m->m_type);
			if (n == NULL) {
				m_freem(n0);
				return NULL;
			}
			alloc++;

			if ((n->m_next = n0) == NULL)
				np = &n->m_next;
			n0 = n;

			*np = m->m_next;
			m->m_next = n0;

			n->m_len = hlen;
			m->m_len = skip;

			m = n;			/* header is at front ... */
			*off = 0;		/* ... of new mbuf */
		}

		IPSEC_STATADD(IPSEC_STAT_MBINSERTED, alloc);
	} else {
		/*
		 * Copy the remainder to the back of the mbuf
		 * so there's space to write the new header.
		 */
		/* XXX can this be memcpy? does it handle overlap? */
		memmove(mtod(m, char *) + skip + hlen,
			mtod(m, char *) + skip, remain);
		m->m_len += hlen;
		*off = skip;
	}

	m0->m_pkthdr.len += hlen;		/* adjust packet length */
	return m;
}

/*
 * m_pad(m, n) pads <m> with <n> bytes at the end. The packet header
 * length is updated, and a pointer to the first byte of the padding
 * (which is guaranteed to be all in one mbuf) is returned.
 */
void *
m_pad(struct mbuf *m, int n)
{
	register struct mbuf *m0, *m1;
	register int len, pad;
	void *retval;

	if (__predict_false(n > MLEN)) {
		panic("%s: %d > MLEN", __func__, n);
	}
	KASSERT(m->m_flags & M_PKTHDR);

	len = m->m_pkthdr.len;
	pad = n;
	m0 = m;

	while (m0->m_len < len) {
		KASSERTMSG(m0->m_next != NULL,
		    "m0 null, len %u m_len %u", len, m0->m_len);
		len -= m0->m_len;
		m0 = m0->m_next;
	}

	if (m0->m_len != len) {
		IPSECLOG(LOG_DEBUG,
		    "length mismatch (should be %d instead of %d)\n",
		    m->m_pkthdr.len, m->m_pkthdr.len + m0->m_len - len);
		m_freem(m);
		return NULL;
	}

	/* Check for zero-length trailing mbufs, and find the last one. */
	for (m1 = m0; m1->m_next; m1 = m1->m_next) {
		if (m1->m_next->m_len != 0) {
			IPSECLOG(LOG_DEBUG,
			    "length mismatch (should be %d instead of %d)\n",
			    m->m_pkthdr.len,
			    m->m_pkthdr.len + m1->m_next->m_len);
			m_freem(m);
			return NULL;
		}

		m0 = m1->m_next;
	}

	if (pad > M_TRAILINGSPACE(m0)) {
		/* Add an mbuf to the chain. */
		MGET(m1, M_DONTWAIT, MT_DATA);
		if (m1 == NULL) {
			m_freem(m);
			IPSECLOG(LOG_DEBUG, "unable to get extra mbuf\n");
			return NULL;
		}

		m0->m_next = m1;
		m0 = m1;
		m0->m_len = 0;
	}

	retval = m0->m_data + m0->m_len;
	m0->m_len += pad;
	m->m_pkthdr.len += pad;

	return retval;
}

/*
 * Remove hlen data at offset skip in the packet.  This is used by
 * the protocols strip protocol headers and associated data (e.g. IV,
 * authenticator) on input.
 */
int
m_striphdr(struct mbuf *m, int skip, int hlen)
{
	struct mbuf *m1;
	int roff;

	KASSERT(m->m_flags & M_PKTHDR);

	/* Find beginning of header */
	m1 = m_getptr(m, skip, &roff);
	if (m1 == NULL)
		return EINVAL;

	/* Remove the header and associated data from the mbuf. */
	if (roff == 0) {
		/* The header was at the beginning of the mbuf */
		IPSEC_STATINC(IPSEC_STAT_INPUT_FRONT);
		m_adj(m1, hlen);
		if (m1 != m)
			m->m_pkthdr.len -= hlen;
	} else if (roff + hlen >= m1->m_len) {
		struct mbuf *mo;
		int adjlen;

		/*
		 * Part or all of the header is at the end of this mbuf,
		 * so first let's remove the remainder of the header from
		 * the beginning of the remainder of the mbuf chain, if any.
		 */
		IPSEC_STATINC(IPSEC_STAT_INPUT_END);
		if (roff + hlen > m1->m_len) {
			adjlen = roff + hlen - m1->m_len;

			/* Adjust the next mbuf by the remainder */
			m_adj(m1->m_next, adjlen);

			/* The second mbuf is guaranteed not to have a pkthdr... */
			m->m_pkthdr.len -= adjlen;
		}

		/* Now, let's unlink the mbuf chain for a second...*/
		mo = m1->m_next;
		m1->m_next = NULL;

		/* ...and trim the end of the first part of the chain...sick */
		adjlen = m1->m_len - roff;
		m_adj(m1, -adjlen);
		if (m1 != m)
			m->m_pkthdr.len -= adjlen;

		/* Finally, let's relink */
		m1->m_next = mo;
	} else {
		/*
		 * The header lies in the "middle" of the mbuf; copy
		 * the remainder of the mbuf down over the header.
		 */
		IPSEC_STATINC(IPSEC_STAT_INPUT_MIDDLE);
		memmove(mtod(m1, u_char *) + roff,
		      mtod(m1, u_char *) + roff + hlen,
		      m1->m_len - (roff + hlen));
		m1->m_len -= hlen;
		m->m_pkthdr.len -= hlen;
	}

	return 0;
}
