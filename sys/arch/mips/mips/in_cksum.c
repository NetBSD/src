/* $NetBSD: in_cksum.c,v 1.10 2003/07/15 02:43:38 lukem Exp $ */

/*
 * Copyright (c) 1993 Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * paragraph appears in all copies of this software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ccsum.c - Highly optimized MIPS checksum function.
 * by Jonathan Kay, Computer Systems Lab, UCSD         4/2/93
 *
 * Version 2.0
 * Techniques and credits:
 *   Basic algorithm is 3-instruction inner loop sum by Peter Desnoyers.
 *   Full word-size reading as described in Usenix W'93 paper.
 *   Pipelined latency absoption technique as described in paper.
 *   Unrolling chosen through testing and examination of actual workload.
 *   Rewrite in 'C' without loss of performance suggested by Vernon Schryver.
 *   15% faster than version 1 ("Usenix version").
 *   150% faster than Ultrix 4.2A checksum routine.
 *
 * BSD changes: Jonathan Stone, Stanford Distributed Systems Group, 1997-08-11
 *
 *   re-written for incremental checksumming of BSD mbufs
 *   and byteswap out-of-phase mbuf sums.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_cksum.c,v 1.10 2003/07/15 02:43:38 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/cdefs.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <machine/endian.h>

union memptr {
	unsigned int *i;
	unsigned long *l;
	unsigned long u;
	unsigned short *s;
	unsigned char *c;
};

static inline uint32_t fastsum(union memptr, int, unsigned int, int);

/*
 * Compute 1's complement sum over a contiguous block at 'buf' for 'n' bytes.
 *
 * Add the resulting checksum into 'oldsum' using 1's complement.
 * 'odd_aligned' is a boolean which if set, indicate the data in 'buf'
 * starts at an odd byte alignment within the containing packet,
 * and so we must byteswap the memory-aligned 1's-complement sum
 * over the data before adding it to `oldsum'.
 */
static inline uint32_t
fastsum(union memptr buf, int n, unsigned int oldsum, int odd_aligned)
{
	unsigned long hilo = 0, high = 0;
	unsigned long w0, w1;
	unsigned int sum = 0;

	/* Align to 32 bits. */
	if (buf.u & 0x3) {
		/* Skip to the end for very small mbufs */
		if (n < 3)
			goto verylittleleft;

		/*
	         * 16-bit-align.
		 * If buf is odd-byte-aligned, add the byte and toggle
		 * our byte-alignment flag.
		 *     If we were odd-aligned on entry, an odd-aligned
		 * byte  makes a 16-bit word with the previous odd byte,
		 * unaligned, making us aligned again.
	 	 *     If we were not already odd-aligned, we are now,
		 * and we must byteswap our 16-bit-aligned sum of
		 *'buf' before accumulating it.
		 */
		if (buf.u & 0x1) {
#if BYTE_ORDER == BIG_ENDIAN
			sum += *(buf.c++);
#else
			sum += (*(buf.c++) << 8);
#endif
			n -= 1;
			odd_aligned = !odd_aligned;
		}

		/* 32-bit-align */
		if (buf.u & 0x2) {
			sum += *(buf.s++);
			n -= 2;
		}
	}

	/* 32-bit-aligned sum.
	   Peter Desnoyers' unbelievable 3-instruction main loop. */
	if (n < 64 + 8)
		goto notmuchleft;
	w0 = buf.l[0];
	w1 = buf.l[1];
	do {
		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[2];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[3];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[4];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[5];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[6];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[7];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[8];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[9];


		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[10];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[11];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[12];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[13];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[14];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[15];

		hilo += w0;
		high += w0 >> 16;
		w0 = buf.l[16];

		hilo += w1;
		high += w1 >> 16;
		w1 = buf.l[17];


		n -= 64;
		buf.c += 64;

	} while (n >= 64 + 8);
	hilo -= (high << 16);
	sum += hilo;
	sum += high;

 notmuchleft:
	high = hilo = 0;
	while (n >= 4) {
		w0 = *(buf.l++);
		hilo += w0;
		high += w0 >> 16;
		n -= 4;
	}
	hilo -= (high << 16);
	sum += hilo;
	sum += high;

	while (n > 1) {
		n -= sizeof(*buf.s);
		sum += *(buf.s++);
	}

 verylittleleft:
	/* handle trailing byte and short (possibly) unaligned payloads */
	while (n-- > 0) {
#if BYTE_ORDER == BIG_ENDIAN
		sum += *buf.c << 8;
#else
		sum += *buf.c;
#endif
	}

	/*
	 * compensate for a trailing byte in previous mbuf
	 * by byteswapping the memory-aligned sum of this mbuf.
 	 */
	if (odd_aligned) {
		sum = (sum & 0xffff) + (sum >> 16);
		sum = (sum & 0xffff) + (sum >> 16);
		sum = oldsum + ((sum >> 8) & 0xff) + ((sum & 0xff) << 8);
	} else {
		/* add upper and lower halfwords together to get full sum */
		sum = oldsum + sum;
		sum = (sum & 0xffff) + (sum >> 16);
	}

	/* fold carry from combining sums */
	sum = (sum & 0xffff) + (sum >> 16);
	return(sum);
}

/*
 * Checksum routine for Internet Protocol family headers.
 *
 */
static inline int
in_cksum_internal(struct mbuf *m, int off, int len, uint32_t sum)
{
	/*u_short **/ union memptr w;
	int mlen;
	int odd_aligned = 0;

	for (; m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w.c = mtod(m, u_char *) + off;
		mlen = m->m_len - off;
		off = 0;
		if (len < mlen)
			mlen = len;
		len -= mlen;

		sum = fastsum(w, mlen, sum, odd_aligned);
		odd_aligned = (odd_aligned + mlen) & 0x01;
	}
	if (len != 0) {
		printf("cksum: out of data, %d\n", len);
	}
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
	uint sum = 0;

	if (nxt != 0) {
		uint16_t *w;
		union {
			struct ipovly ipov;
			u_int16_t w[10];
		} u;

		/* pseudo header */
		memset(&u.ipov, 0, sizeof(u.ipov));
		u.ipov.ih_len = htons(len);
		u.ipov.ih_pr = nxt;
		u.ipov.ih_src = mtod(m, struct ip *)->ip_src;
		u.ipov.ih_dst = mtod(m, struct ip *)->ip_dst;
		w = u.w;
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
