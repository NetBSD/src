/*	$NetBSD: in_cksum.c,v 1.3 2001/06/13 06:01:50 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

/*
 * Checksum routine for Internet Protocol family headers.
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 *
 * PowerPC version.
 */

#define	REDUCE1		sum = (sum & 0xffff) + (sum >> 16)
/* Two REDUCE1s is faster than REDUCE1; if (sum > 65535) sum -= 65536; */
#define	REDUCE		{ REDUCE1; REDUCE1; }

static __inline__ int
in_cksum_internal(struct mbuf *m, int off, int len, u_int sum)
{
	uint8_t *w;
	int mlen = 0;
	int byte_swapped = 0;

	union {
		uint8_t  c[2];
		uint16_t s;
	} s_util;

	for (;m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w = mtod(m, uint8_t *) + off;

		/*
		 * 'off' can only be non-zero on the first pass of this
		 * loop when mlen != -1, so we don't need to worry about
		 * 'off' in the if clause below.
		 */
		if (mlen == -1) {
			/*
			 * The first byte of this mbuf is the continuation
			 * of a word spanning between this mbuf and the
			 * last mbuf.
			 *
			 * s_util.c[0] is already saved when scanning previous 
			 * mbuf.
			 */
			s_util.c[1] = *w++;
			sum += s_util.s;
			mlen = m->m_len - 1;
			len--;
		} else {
			mlen = m->m_len - off;
			off = 0;
		}
		if (len < mlen)
			mlen = len;
		len -= mlen;

		/*
		 * Force to a word boundary.
		 */
		if ((3 & (long) w) && (mlen > 0)) {
			if ((1 & (long) w) && (mlen > 0)) {
				REDUCE;
				sum <<= 8;
				s_util.c[0] = *w++;
				mlen--;
				byte_swapped = 1;
			}
			if ((2 & (long) w) && (mlen > 1)) {
				sum += *(uint16_t *)w;
				w += 2;
				mlen -= 2;
			}
		}

		if (mlen >= 64) {
			register int n __asm("r0");
			uint8_t *tmpw;

			n = mlen >> 6;
			tmpw = w - 4;
			asm volatile(
				"addze 7,7;"		/* clear carry */
				"mtctr %1;"		/* load loop count */
				"1:"
				"lwzu 7,4(%2);"		/* load current data word */
				"lwzu 8,4(%2);"
				"lwzu 9,4(%2);"
				"lwzu 10,4(%2);"
				"adde %0,%0,7;"		/* add to sum */
				"adde %0,%0,8;"
				"adde %0,%0,9;"
				"adde %0,%0,10;"
				"lwzu 7,4(%2);"
				"lwzu 8,4(%2);"
				"lwzu 9,4(%2);"
				"lwzu 10,4(%2);"
				"adde %0,%0,7;"
				"adde %0,%0,8;"
				"adde %0,%0,9;"
				"adde %0,%0,10;"
				"lwzu 7,4(%2);"
				"lwzu 8,4(%2);"
				"lwzu 9,4(%2);"
				"lwzu 10,4(%2);"
				"adde %0,%0,7;"
				"adde %0,%0,8;"
				"adde %0,%0,9;"
				"adde %0,%0,10;"
				"lwzu 7,4(%2);"
				"lwzu 8,4(%2);"
				"lwzu 9,4(%2);"
				"lwzu 10,4(%2);"
				"adde %0,%0,7;"
				"adde %0,%0,8;"
				"adde %0,%0,9;"
				"adde %0,%0,10;"
				"bdnz 1b;"		/* loop */
				"addze %0,%0;"		/* add carry bit */
				: "+r"(sum)
				: "r"(n), "r"(tmpw)
				: "7", "8", "9", "10");	/* clobber r7, r8, r9, r10 */
			w += n * 64;
			mlen -= n * 64;
		}

		if (mlen >= 8) {
			register int n __asm("r0");
			uint8_t *tmpw;

			n = mlen >> 3;
			tmpw = w - 4;
			asm volatile(
				"addze %1,%1;"		/* clear carry */
				"mtctr %1;"		/* load loop count */
				"1:"
				"lwzu 7,4(%2);"		/* load current data word */
				"lwzu 8,4(%2);"
				"adde %0,%0,7;"		/* add to sum */
				"adde %0,%0,8;"
				"bdnz 1b;"		/* loop */
				"addze %0,%0;"		/* add carry bit */
				: "+r"(sum)
				: "r"(n), "r"(tmpw)
				: "7", "8");		/* clobber r7, r8 */
			w += n * 8;
			mlen -= n * 8;
		}

		if (mlen == 0 && byte_swapped == 0)
			continue;
		REDUCE;

		while ((mlen -= 2) >= 0) {
			sum += *(uint16_t *)w;
			w += 2;
		}

		if (byte_swapped) {
			REDUCE;
			sum <<= 8;
			byte_swapped = 0;
			if (mlen == -1) {
				s_util.c[1] = *w;
				sum += s_util.s;
				mlen = 0;
			} else
				mlen = -1;
		} else if (mlen == -1)
			s_util.c[0] = *w;
	}
	if (len)
		printf("cksum: out of data\n");
	if (mlen == -1) {
		/* The last mbuf has odd # of bytes. Follow the
		   standard (the odd byte may be shifted left by 8 bits
		   or not as determined by endian-ness of the machine) */
		s_util.c[1] = 0;
		sum += s_util.s;
	}
	REDUCE;
	return (~sum & 0xffff);
}

int
in_cksum(struct mbuf *m, int len)
{
	return (in_cksum_internal(m, 0, len, 0));
}

int
in4_cksum(struct mbuf *m, uint8_t nxt, int off, int len)
{
	uint16_t *w;
	u_int sum = 0;
	struct ipovly ipov;

	if (nxt != 0) {
		/* pseudo header */
		memset(&ipov, 0, sizeof(ipov));
		ipov.ih_len = htons(len);
		ipov.ih_pr = nxt; 
		ipov.ih_src = mtod(m, struct ip *)->ip_src; 
		ipov.ih_dst = mtod(m, struct ip *)->ip_dst;
		w = (uint16_t *)&ipov;
		/* assumes sizeof(ipov) == 20 */
		sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3]; sum += w[4];
		sum += w[5]; sum += w[6]; sum += w[7]; sum += w[8]; sum += w[9];
	}

	/* skip unnecessary part */
	while (m && off > 0) {
		if (m->m_len > off)
			break;
		off -= m->m_len;
		m = m->m_next;
	}

	return (in_cksum_internal(m, off, len, sum));
}
