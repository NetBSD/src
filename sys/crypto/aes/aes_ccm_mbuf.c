/*	$NetBSD: aes_ccm_mbuf.c,v 1.1 2020/07/25 22:15:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_ccm_mbuf.c,v 1.1 2020/07/25 22:15:55 riastradh Exp $");

#include <sys/types.h>

#include <sys/mbuf.h>

#include <lib/libkern/libkern.h>

#include <crypto/aes/aes_ccm.h>
#include <crypto/aes/aes_ccm_mbuf.h>

void
aes_ccm_enc_mbuf(struct aes_ccm *C, struct mbuf *m, size_t off, size_t len,
    void *tag)
{
	uint8_t *p;
	size_t seglen;

	while (off >= m->m_len) {
		KASSERT(m->m_next);
		m = m->m_next;
		off -= m->m_len;
	}

	for (; len > 0; m = m->m_next, off = 0, len -= seglen) {
		KASSERT(m->m_len >= off);
		p = mtod(m, uint8_t *) + off;
		seglen = MIN(m->m_len - off, len);
		aes_ccm_enc(C, p, p, seglen);
	}

	aes_ccm_tag(C, tag);
}

int
aes_ccm_dec_mbuf(struct aes_ccm *C, struct mbuf *m0, size_t off0, size_t len0,
    const void *tag)
{
	struct mbuf *m;
	size_t off, len;
	uint8_t *p;
	size_t seglen;

	while (off0 >= m0->m_len) {
		KASSERT(m0->m_next);
		m0 = m0->m_next;
		off0 -= m0->m_len;
	}

	for (m = m0, off = off0, len = len0;
	     len > 0;
	     m = m->m_next, off = 0, len -= seglen) {
		KASSERT(m->m_len >= off);
		p = mtod(m, uint8_t *) + off;
		seglen = MIN(m->m_len - off, len);
		aes_ccm_dec(C, p, p, seglen);
	}

	if (aes_ccm_verify(C, tag))
		return 1;

	/*
	 * Verification failed.  Zero the mbuf so we don't accidentally
	 * leak anything about it.
	 */
	for (m = m0, off = off0, len = len0;
	     len > 0;
	     m = m->m_next, off = 0, len -= seglen) {
		KASSERT(m->m_len >= off);
		p = mtod(m, uint8_t *) + off;
		seglen = MIN(m->m_len - off, len);
		memset(p, 0, seglen);
	}

	return 0;
}
