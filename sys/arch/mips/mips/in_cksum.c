/*	$NetBSD: in_cksum.c,v 1.2 1997/07/22 07:36:18 jonathan Exp $	*/
 
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/cdefs.h>
#include <netinet/in.h>

union memptr {
	unsigned int *i;
	unsigned long *l;
	unsigned long u;
	unsigned short *s;
	unsigned char *c;
};

static __inline u_int32_t fastsum __P((union memptr, int n, u_int sum, int odd));


u_int32_t
fastsum(buf, n, oldsum, odd)
	union memptr buf;
	int n;
	unsigned int oldsum;
{
	unsigned long hilo = 0, high = 0;
	unsigned long w0, w1;
	register unsigned int sum = 0;

	if (buf.u & 0x3) {
		/* 16-bit-align. Have to do this first so short buffers done right. */
		if (buf.u & 0x1) {
			sum += (*(buf.c++) << 8);
			n -= 1;
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
	
	/* handle trailing byte */
	if (n > 0)
		sum += *buf.c;

	/*
	 * compensate for a trailing byte in previous mbuf
	 * by byteswapping the aligned sum of this mbuf.
 	 */
	if (odd & 0x01) {
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
 * Checksum routine for Internet Protocol family headers (mips r3000 Version).
 *
 */
int
in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register /*u_short **/ union memptr w;
	register u_int32_t sum = 0;
	register int mlen;
	register int odd = 0;

	for ( ; m && len; m = m->m_next) {

		mlen = m->m_len;
		if (mlen == 0)
			continue;
		if (mlen > len)
			mlen = len;
		w.c = mtod(m, u_char *);
		sum = fastsum(w, mlen, sum, odd);
		len -= mlen;
		odd += mlen;
	}
	if (len != 0) {
		printf("in_cksum: out of data, %d\n", len);
	}
	return (~sum & 0xffff);
}
