/*	$NetBSD: in_cksum_arm.c,v 1.6 2003/08/07 16:26:52 agc Exp $	*/

/*
 * ARM version:
 *
 * Copyright (c) 1997 Mark Brinicome
 * Copyright (c) 1997 Causality Limited
 *
 * Based on the sparc version.
 */

/*
 * Sparc version:
 *
 * Copyright (c) 1992, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)in_cksum.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Sparc version:
 *
 * Copyright (c) 1995 Zubin Dittia.
 * Copyright (c) 1995 Matthew R. Green.
 * Copyright (c) 1994 Charles M. Hannum.
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
 *	@(#)in_cksum.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_cksum_arm.c,v 1.6 2003/08/07 16:26:52 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

/*
 * Checksum routine for Internet Protocol family headers.
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 *
 * ARM version.
 */

#define ADD64	__asm __volatile("	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adds	%1,%7,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adcs	%1,%1,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adcs	%1,%1,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adcs	%1,%1,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	adcs	%1,%1,#0\n"		\
	: "=r" (w), "=r" (sum), "=&r" (tmp1), "=&r" (tmp2), \
	  "=&r" (tmp3), "=&r" (tmp4) \
	: "0" (w), "r" (sum)	\
	: "cc")
	
#define ADD32	__asm __volatile("	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adds	%1,%7,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adcs	%1,%1,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	adcs	%1,%1,#0\n"		\
	: "=r" (w), "=r" (sum), "=&r" (tmp1), "=&r" (tmp2), \
	  "=&r" (tmp3), "=&r" (tmp4) \
	: "0" (w), "r" (sum)	\
	: "cc")

#define ADD16	__asm __volatile("	\n\
	ldmia	%0!, {%2, %3, %4, %5}	\n\
	adds	%1,%7,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,%4; adcs %1,%1,%5	\n\
	adcs	%1,%1,#0\n"		\
	: "=r" (w), "=r" (sum), "=&r" (tmp1), "=&r" (tmp2), \
	  "=&r" (tmp3), "=&r" (tmp4) \
	: "0" (w), "r" (sum)	\
	: "cc")

#define ADD8	__asm __volatile("	\n\
	ldmia	%0!, {%2, %3}		\n\
	adds	%1,%5,%2; adcs %1,%1,%3	\n\
	adcs	%1,%1,#0\n"		\
	: "=r" (w), "=r" (sum), "=&r" (tmp1), "=&r" (tmp2)	\
	: "0" (w), "r" (sum)		\
	: "cc" )

#define ADD4	__asm __volatile("	\n\
	ldr	%2,[%0],#4		\n\
	adds	%1,%4,%2		\n\
	adcs	%1,%1,#0\n"		\
	: "=r" (w), "=r" (sum), "=&r" (tmp1) \
	: "0" (w), "r" (sum)		\
	: "cc")

/*#define REDUCE	{sum = (sum & 0xffff) + (sum >> 16);}*/
#define REDUCE	__asm __volatile("	\n\
	mov	%2, #0x00ff		\n\
	orr	%2, %2, #0xff00		\n\
	and	%2, %0, %2		\n\
	add	%0, %2, %0, lsr #16\n"	\
	: "=r" (sum)			\
	: "0" (sum), "r" (tmp1))

#define ADDCARRY	{if (sum > 0xffff) sum -= 0xffff;}
#define ROL		{sum = sum << 8;}	/* depends on recent REDUCE */
#define ADDBYTE		{ROL; sum += (*w << 8); byte_swapped ^= 1;}
#define ADDSHORT	{sum += *(u_short *)w;}
#define ADVANCE(n)	{w += n; mlen -= n;}
#define ADVANCEML(n)	{mlen -= n;}

static __inline__ int
in_cksum_internal(struct mbuf *m, int off, int len, u_int sum)	
{
	u_char *w;
	int mlen = 0;
	int byte_swapped = 0;

	/*
	 * Declare four temporary registers for use by the asm code.  We
	 * allow the compiler to pick which specific machine registers to
	 * use, instead of hard-coding this in the asm code above.
	 */
	register u_int tmp1, tmp2, tmp3, tmp4;

	for (; m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w = mtod(m, u_char *) + off;
		mlen = m->m_len - off;
		off = 0;
		if (len < mlen)
			mlen = len;
		len -= mlen;

		/*
		 * Ensure that we're aligned on a word boundary here so
		 * that we can do 32 bit operations below.
		 */
		if ((3 & (long)w) != 0) {
			REDUCE;
			if ((1 & (long)w) != 0 && mlen >= 1) {
				ADDBYTE;
				ADVANCE(1);
			}
			if ((2 & (long)w) != 0 && mlen >= 2) {
				ADDSHORT;
				ADVANCE(2);
			}
		}

		/*
		 * Do as many 32 bit operations as possible using the
		 * 64/32/16/8/4 macro's above, using as many as possible of
		 * these.
		 */

		while (mlen >= 64) {
			ADD64;
			ADVANCEML(64);
		}
		if (mlen >= 32) {
			ADD32;
			ADVANCEML(32);
		}
		if (mlen >= 16) {
			ADD16;
			ADVANCEML(16);
		}
		if (mlen >= 8) {
			ADD8;
			ADVANCEML(8);
		}
		if (mlen >= 4) {
			ADD4;
			ADVANCEML(4)
		}
		if (mlen == 0)
			continue;

		REDUCE;
		if (mlen >= 2) {
			ADDSHORT;
			ADVANCE(2);
		}
		if (mlen == 1) {
			ADDBYTE;
		}
	}
	if (byte_swapped) {
		REDUCE;
		ROL;
	}
	REDUCE;
	ADDCARRY;

	return (0xffff ^ sum);
}

int
in_cksum(m, len)
	struct mbuf *m;
	int len;
{

	return (in_cksum_internal(m, 0, len, 0));
}

int
in4_cksum(m, nxt, off, len)
	struct mbuf *m;
	u_int8_t nxt;
	int off, len;
{
	u_int sum = 0;
	
	if (nxt != 0) {
		/* for ADD macros */
		register u_int tmp1, tmp2, tmp3, tmp4;
		u_char *w;
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
		w = (u_char *)&ipov;

		/* assumes sizeof(ipov) == 20 */
		ADD16;
		ADD4;
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
