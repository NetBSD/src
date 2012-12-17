/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(0, "$NetBSD: cpu_in_cksum_neon.c,v 1.1 2012/12/17 00:44:03 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/ip.h>

uint32_t cpu_in_cksum_neon(const void *, size_t);
uint32_t cpu_in_cksum_neon_v4hdr(const void *);

int
cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t initial_sum)
{
	uint32_t csum = initial_sum;
	int odd = 0;

	/*
	 * Taken control of the NEON PCU.
	 */
	vfp_hijack();

	/*
	 * Fast path for the normal ip_header
	 */
	if (off == 0
	    && csum == 0
	    && len == sizeof(struct ip)
	    && ((uintptr_t)m->m_data & 3) == 0
	    && m->m_len >= len) {
		csum = cpu_in_cksum_neon_v4hdr(m->m_data);

		/*
		 * We are now down with NEON.
		 */
		vfp_surrender();

		if (csum == 0x10000)	/* note 0x10000 - 0xffff == 1 */
			return 1;
		return csum == 0 ? 0xffff : csum;	/* never return 0. */
	}

	/*
	 * Skip the initial mbufs
	 */
	while (m->m_len >= off) {
		m = m->m_next;
		off -= m->m_len;
		KASSERT(m != NULL);
	}

	for (; len > 0; m = m->m_next, off = 0) {
		KASSERT(m != NULL);
		int dlen = MIN(m->m_len - off, len);
		const void *dptr = m->m_data + off;
		/*
		 * This routine will add based on the memory layout so
		 * if the previous len was odd or the this buffer starts
		 * on an odd address, shift the csum by 8 so its properly
		 * aligned.  It will be taken care of when we do the final
		 * checksum fold.
		 */
		uint32_t tmpsum = cpu_in_cksum_neon(dptr, dlen);
		if (odd ^ ((uint32_t)dptr & 1))
			tmpsum <<= 8;
		/*
		 * Accumulate checksum, folding will be done later
		 */
		csum += tmpsum;
		odd ^= dlen & 1;
		len -= dlen;
	}

	/*
	 * We are now down with NEON.
	 */
	vfp_surrender();

	/*
	 * Time to fold the checksum
	 */
	csum = (csum >> 16) + (csum & 0xffff);
	/*
	 * Now it could be 0x1xxxx so fold again
	 */
	csum = (csum >> 16) + (csum & 0xffff);

	KASSERT(csum <= 0x10000);
	if (csum == 0x10000)	/* note 0x10000 - 0xffff == 1 */
		return 1;
	return csum == 0 ? 0xffff : csum;	/* never return 0. */
}
