/*	$NetBSD: cpu_in_cksum.c,v 1.1.2.2 2010/10/09 03:31:53 yamt Exp $ */

/*
 * Copyright (c) 1995 Matthew R. Green.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, and it's contributors.
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
 * Copyright (c) 1995 Zubin Dittia.
 * Copyright (c) 1994, 1998 Charles M. Hannum.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, and it's contributors.
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
__KERNEL_RCSID(0, "$NetBSD: cpu_in_cksum.c,v 1.1.2.2 2010/10/09 03:31:53 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <netinet/in.h>

/*
 * Checksum routine for Internet Protocol family headers.
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 *
 * SPARC version.
 */

/*
 * The checksum computation code here is significantly faster than its
 * vanilla C counterpart (by significantly, I mean 2-3 times faster if
 * the data is in cache, and 1.5-2 times faster if the data is not in
 * cache).
 * We optimize on three fronts:
 *	1. By using the add-with-carry (addxcc) instruction, we can use
 *	   32-bit operations instead of 16-bit operations.
 *	2. By unrolling the main loop to reduce branch overheads.
 *	3. By doing a sequence of load,load,add,add,load,load,add,add,
 *	   we can avoid the extra stall cycle which is incurred if the
 *	   instruction immediately following a load tries to use the
 *	   target register of the load.
 * Another possible optimization is to replace a pair of 32-bit loads
 * with a single 64-bit load (ldd) instruction, but I found that although
 * this improves performance somewhat on Sun4c machines, it actually
 * reduces performance considerably on Sun4m machines (I don't know why).
 * So I chose to leave it out.
 *
 * Zubin Dittia (zubin@dworkin.wustl.edu)
 */

#define Asm	asm volatile
#define ADD64		Asm("	ld [%4+ 0],%1;   ld [%4+ 4],%2;		\
				addcc  %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+ 8],%1;   ld [%4+12],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+16],%1;   ld [%4+20],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+24],%1;   ld [%4+28],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+32],%1;   ld [%4+36],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+40],%1;   ld [%4+44],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+48],%1;   ld [%4+52],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+56],%1;   ld [%4+60],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				addxcc %0,0,%0"				\
				: "=r" (sum), "=&r" (tmp1), "=&r" (tmp2)\
				: "0" (sum), "r" (w))
#define ADD32		Asm("	ld [%4+ 0],%1;   ld [%4+ 4],%2;		\
				addcc  %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+ 8],%1;   ld [%4+12],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+16],%1;   ld [%4+20],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+24],%1;   ld [%4+28],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				addxcc %0,0,%0"				\
				: "=r" (sum), "=&r" (tmp1), "=&r" (tmp2)\
				: "0" (sum), "r" (w))
#define ADD16		Asm("	ld [%4+ 0],%1;   ld [%4+ 4],%2;		\
				addcc  %0,%1,%0; addxcc %0,%2,%0;	\
				ld [%4+ 8],%1;   ld [%4+12],%2;		\
				addxcc %0,%1,%0; addxcc %0,%2,%0;	\
				addxcc %0,0,%0"				\
				: "=r" (sum), "=&r" (tmp1), "=&r" (tmp2)\
				: "0" (sum), "r" (w))
#define ADD8		Asm("	ld [%4+ 0],%1;   ld [%4+ 4],%2;		\
				addcc  %0,%1,%0; addxcc %0,%2,%0;	\
				addxcc %0,0,%0"				\
				: "=r" (sum), "=&r" (tmp1), "=&r" (tmp2)\
				: "0" (sum), "r" (w))
#define ADD4		Asm("	ld [%3+ 0],%1; 				\
				addcc  %0,%1,%0;			\
				addxcc %0,0,%0"				\
				: "=r" (sum), "=&r" (tmp1)		\
				: "0" (sum), "r" (w))

#define REDUCE		{sum = (sum & 0xffff) + (sum >> 16);}
#define ADDCARRY	{if (sum > 0xffff) sum -= 0xffff;}
#define ROL		{sum = sum << 8;}	/* depends on recent REDUCE */
#define ADDBYTE		{ROL; sum += *w; byte_swapped ^= 1;}
#define ADDSHORT	{sum += *(uint16_t *)w;}
#define ADVANCE(n)	{w += n; mlen -= n;}

int
cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t sum)
{
	uint8_t *w;
	int mlen = 0;
	int byte_swapped = 0;

	/*
	 * Declare two temporary registers for use by the asm code.  We
	 * allow the compiler to pick which specific machine registers to
	 * use, instead of hard-coding this in the asm code above.
	 */
	uint32_t tmp1, tmp2;

	for (; m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w = mtod(m, uint8_t *) + off;
		mlen = m->m_len - off;
		off = 0;
		if (len < mlen)
			mlen = len;
		len -= mlen;

		/*
		 * Ensure that we're aligned on a word boundary here so
		 * that we can do 32 bit operations below.
		 */
		if (((uintptr_t)w & 3) != 0) {
			REDUCE;
			if (((uintptr_t)w & 1) != 0 && mlen >= 1) {
				ADDBYTE;
				ADVANCE(1);
			}
			if (((uintptr_t)w & 2) != 0 && mlen >= 2) {
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
			ADVANCE(64);
		}
		if (mlen >= 32) {
			ADD32;
			ADVANCE(32);
		}
		if (mlen >= 16) {
			ADD16;
			ADVANCE(16);
		}
		if (mlen >= 8) {
			ADD8;
			ADVANCE(8);
		}
		if (mlen >= 4) {
			ADD4;
			ADVANCE(4)
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

	return 0xffff ^ sum;
}
