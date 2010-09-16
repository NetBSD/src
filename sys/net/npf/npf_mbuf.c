/*	$NetBSD: npf_mbuf.c,v 1.2 2010/09/16 04:53:27 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * NPF network buffer management interface.
 *
 * Network buffer in NetBSD is mbuf.  Internal mbuf structures are
 * abstracted within this source.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_mbuf.c,v 1.2 2010/09/16 04:53:27 rmind Exp $");
#endif

#include <sys/param.h>
#include <sys/mbuf.h>

#include "npf_impl.h"

/*
 * nbuf_dataptr: return a pointer to data in nbuf.
 */
void *
nbuf_dataptr(nbuf_t *nbuf)
{
	const struct mbuf *m = nbuf;

	return mtod(m, void *);
}

/*
 * nbuf_advance: advance in mbuf or chain by specified amount of bytes.
 *
 * => Returns new pointer to data in mbuf and NULL if offset gets invalid.
 * => Sets nbuf to current (after advance) mbuf in the chain.
 */
void *
nbuf_advance(nbuf_t **nbuf, void *n_ptr, u_int n)
{
	struct mbuf *m = *nbuf;
	u_int off, wmark;
	uint8_t *d;

	/* Offset with amount to advance. */
	off = (uintptr_t)n_ptr - mtod(m, uintptr_t) + n;
	wmark = m->m_len;

	/* Find the mbuf according to offset. */
	while (__predict_false(wmark <= off)) {
		m = m->m_next;
		if (__predict_false(m == NULL)) {
			/*
			 * If out of chain, then offset is
			 * higher than packet length.
			 */
			return NULL;
		}
		wmark += m->m_len;
	}

	/* Offset in mbuf data. */
	d = mtod(m, uint8_t *);
	KASSERT(off >= (wmark - m->m_len));
	d += (off - (wmark - m->m_len));

	*nbuf = (void *)m;
	return d;
}

/*
 * nbuf_rw_datum: read or write a datum of specified length at current
 * offset in the nbuf chain and copy datum into passed buffer.
 *
 * => Datum is allowed to overlap between two or more mbufs.
 * => Note: all data in nbuf is in network byte order.
 * => Returns 0 on success, error code on failure.
 *
 * Note: this function must be static inline with constant operation
 * parameter - we expect constant propagation.
 */

#define	NBUF_DATA_READ		0
#define	NBUF_DATA_WRITE		1

static inline int
nbuf_rw_datum(const int wr, nbuf_t *nbuf, void *n_ptr, size_t len, void *buf)
{
	uint8_t *d = n_ptr, *b = buf;
	struct mbuf *m = nbuf;
	u_int off, wmark, end;

	/* Current offset in mbuf. */
	off = (uintptr_t)n_ptr - mtod(m, uintptr_t);
	KASSERT(off < m->m_len);
	wmark = m->m_len;

	/* Is datum overlapping? */
	end = off + len;
	while (__predict_false(end > wmark)) {
		u_int l;

		/* Get the part of current mbuf. */
		l = m->m_len - off;
		KASSERT(l < len);
		len -= l;
		if (wr == NBUF_DATA_WRITE) {
			while (l--)
				*d++ = *b++;
		} else {
			KASSERT(wr == NBUF_DATA_READ);
			while (l--)
				*b++ = *d++;
		}
		KASSERT(len > 0);

		/* Take next mbuf and continue. */
		m = m->m_next;
		if (__predict_false(m == NULL)) {
			/*
			 * If out of chain, then offset with datum
			 * length exceed the packet length.
			 */
			return EINVAL;
		}
		wmark += m->m_len;
		d = mtod(m, uint8_t *);
		off = 0;
	}
	KASSERT(n_ptr == d || mtod(m, uint8_t *) == d);
	KASSERT(len <= m->m_len);

	/* Non-overlapping case: fetch the actual data. */
	if (wr == NBUF_DATA_WRITE) {
		while (len--)
			*d++ = *b++;
	} else {
		KASSERT(wr == NBUF_DATA_READ);
		while (len--)
			*b++ = *d++;
	}
	return 0;
}

/*
 * nbuf_{fetch|store}_datum: read/write absraction calls on nbuf_rw_datum().
 */
int
nbuf_fetch_datum(nbuf_t *nbuf, void *n_ptr, size_t len, void *buf)
{

	return nbuf_rw_datum(NBUF_DATA_READ, nbuf, n_ptr, len, buf);
}

int
nbuf_store_datum(nbuf_t *nbuf, void *n_ptr, size_t len, void *buf)
{

	return nbuf_rw_datum(NBUF_DATA_WRITE, nbuf, n_ptr, len, buf);
}

/*
 * nbuf_add_tag: add a tag to specified network buffer.
 *
 * => Returns 0 on success, or errno on failure.
 */
int
nbuf_add_tag(nbuf_t *nbuf, uint32_t key, uint32_t val)
{
	struct mbuf *m = nbuf;
	struct m_tag *mt;
	uint32_t *dat;

	mt = m_tag_get(PACKET_TAG_NPF, sizeof(uint32_t), M_NOWAIT);
	if (__predict_false(mt == NULL)) {
		return ENOMEM;
	}
	dat = (uint32_t *)(mt + 1);
	*dat = val;
	m_tag_prepend(m, mt);
	return 0;
}

/*
 * nbuf_find_tag: find a tag in specified network buffer.
 *
 * => Returns 0 on success, or errno on failure.
 */
int
nbuf_find_tag(nbuf_t *nbuf, uint32_t key, void **data)
{
	struct mbuf *m = nbuf;
	struct m_tag *mt;

	mt = m_tag_find(m, PACKET_TAG_NPF, NULL);
	if (__predict_false(mt == NULL)) {
		return EINVAL;
	}
	*data = (void *)(mt + 1);
	return 0;
}
