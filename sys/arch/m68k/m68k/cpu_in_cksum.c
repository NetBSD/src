/*	$NetBSD: cpu_in_cksum.c,v 1.1.4.2 2010/10/22 07:21:23 uebayasi Exp $	*/
/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_in_cksum.c,v 1.1.4.2 2010/10/22 07:21:23 uebayasi Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>

#include <netinet/in.h>

/*
 * Taken from sys/netinet/cpu_in_cksum.c and modified to use old MD oc_cksum().
 */

/* in <m68k/m68k/oc_cksum.s> */
extern int oc_cksum(uint8_t *, int, uint32_t);

int
cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t initial_sum)
{
	int mlen;
	uint32_t sum, partial;
	unsigned int final_acc;
	uint8_t *data;
	bool needs_swap, started_on_odd;

	KASSERT(len >= 0);
	KASSERT(off >= 0);

	needs_swap = false;
	started_on_odd = false;
	sum = (initial_sum >> 16) + (initial_sum & 0xffff);

	for (;;) {
		if (__predict_false(m == NULL)) {
			printf("in_cksum: out of data\n");
			return -1;
		}
		mlen = m->m_len;
		if (mlen > off) {
			mlen -= off;
			data = mtod(m, uint8_t *) + off;
			goto post_initial_offset;
		}
		off -= mlen;
		if (len == 0)
			break;
		m = m->m_next;
	}

	for (; len > 0; m = m->m_next) {
		if (__predict_false(m == NULL)) {
			printf("in_cksum: out of data\n");
			return -1;
		}
		mlen = m->m_len;
		data = mtod(m, uint8_t *);
 post_initial_offset:
		if (mlen == 0)
			continue;
		if (mlen > len)
			mlen = len;
		len -= mlen;

		partial = 0;
		if ((uintptr_t)data & 1) {
			/* Align on word boundary */
			started_on_odd = !started_on_odd;
			/* We are big endian */
			partial = *data;
			++data;
			--mlen;
		}
		needs_swap = started_on_odd;

		/* A possbile trailing odd byte is handled in oc_cksum() */
		if (mlen & 1)
			started_on_odd = !started_on_odd;
		/* 16 bit word alignment is enough on m68k */
		partial = oc_cksum(data, mlen, partial);

		if (needs_swap)
			partial = (partial << 8) + (partial >> 24);
		sum += (partial >> 16) + (partial & 0xffff);
		/*
		 * Reduce sum to allow potential byte swap
		 * in the next iteration without carry.
		 */
		sum = (sum >> 16) + (sum & 0xffff);
	}
	final_acc = ((sum >> 16) & 0xffff) + (sum & 0xffff);
	final_acc = (final_acc >> 16) + (final_acc & 0xffff);
	return ~final_acc & 0xffff;
}
