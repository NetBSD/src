/*	$NetBSD: in4_cksum.c,v 1.1 2000/06/07 19:31:33 matt Exp $	*/

/*
 * Copyright (C) 1999 WIDE Project.
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
 * Copyright (c) 1988, 1992, 1993
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
 *	@(#)in_cksum.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

/*
 * Checksum routine for Internet Protocol family headers.
 * This is only for IPv4 pseudo header checksum.
 * No need to clear non-pseudo-header fields in IPv4 header.
 * len is for actual payload size, and does not include IPv4 header and
 * skipped header chain (off + len should be equal to the whole packet).
 *
 * This implementation is VAX version.
 */

#define REDUCE		{sum = (sum & 0xffff) + (sum >> 16);}
#define ADDCARRY	{if (sum > 0xffff) sum -= 0xffff;}
#define ADVANCE(n)	{w += n; mlen -= n;}
#define SWAP		{sum <<= 8;}		/* depends on recent REDUCE */

#define Asm     __asm __volatile
#define ADDL    Asm("addl2 (%2)+,%0" : "=r" (sum) : "0" (sum), "r" (w))
#define ADWC    Asm("adwc  (%2)+,%0" : "=r" (sum) : "0" (sum), "r" (w))
#define ADDC    Asm("adwc     $0,%0" : "=r" (sum) : "0" (sum))
#define UNSWAP  Asm("rotl  $8,%0,%0" : "=r" (sum) : "0" (sum))
#define ADDBYTE	{sum += *w; SWAP; byte_swapped ^= 1;}
#define ADDWORD	{sum += *(u_short *)w;}

int
in4_cksum(struct mbuf *m, u_int8_t nxt, int off, int len)
{
	u_int8_t *w;
	u_int32_t sum = 0;
	int mlen = 0;
	int byte_swapped = 0;

	if (off > 0) {
		struct ipovly ipov;
		/* pseudo header */
		if (off < sizeof(struct ipovly))
			panic("in4_cksum: offset too short");
		if (m->m_len < sizeof(struct ip))
			panic("in4_cksum: bad mbuf chain");
		bzero(&ipov, sizeof(ipov));
		ipov.ih_len = htons(len);
		ipov.ih_pr = nxt;
		ipov.ih_src = mtod(m, struct ip *)->ip_src;
		ipov.ih_dst = mtod(m, struct ip *)->ip_dst;
		w = (u_int8_t *)&ipov;

		/* assumes sizeof(ipov) == 20 */
		ADDL;	ADWC;	ADWC;	ADWC;
		ADWC;	ADDC;

		/* skip unnecessary part */
		while (m && off > 0) {
			if (m->m_len > off)
				break;
			off -= m->m_len;
			m = m->m_next;
		}
	}

	for (;m && len; m = m->m_next) {
		if ((mlen = m->m_len) == 0)
			continue;
		w = mtod(m, u_int8_t *) + off;
		off = 0;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		if (mlen < 16)
			goto short_mbuf;
		/*
		 * Ensure that we're aligned on a word boundary here so
		 * that we can do 32 bit operations below.
		 */
		if ((3 & (intptr_t) w) != 0) {
			REDUCE;
			if ((1 & (intptr_t) w) != 0) {
				ADDBYTE;
				ADVANCE(1);
			}
			if ((2 & (intptr_t) w) != 0) {
				ADDWORD;
				ADVANCE(2);
			}
		}
		/*
		 * Do as much of the checksum as possible 32 bits at at time.
		 * In fact, this loop is unrolled to make overhead from
		 * branches &c small.
		 */
		while ((mlen -= 32) >= 0) {
			/*
			 * Add with carry 16 words and fold in the last carry
			 * by adding a 0 with carry.
			 */
			ADDL;	ADWC;	ADWC;	ADWC;
			ADWC;	ADWC;	ADWC;	ADWC;
			ADDC;
		}
		mlen += 32;
		if (mlen >= 16) {
			ADDL;	ADWC;	ADWC;	ADWC;
			ADDC;
			mlen -= 16;
		}
	short_mbuf:
		if (mlen >= 8) {
			ADDL;	ADWC;
			ADDC;
			mlen -= 8;
		}
		if (mlen >= 4) {
			ADDL;
			ADDC;
			mlen -= 4;
		}
		if (mlen > 0) {
			REDUCE;
			if (mlen >= 2) {
				ADDWORD;
				ADVANCE(2);
			}
			if (mlen >= 1) {
				ADDBYTE;
			}
		}
	}

	if (len)
		printf("cksum4: out of data\n");
	if (byte_swapped) {
		UNSWAP;
	}
	REDUCE;
	ADDCARRY;
	return (sum ^ 0xffff);
}
