/*	$NetBSD: in_cksum.S,v 1.1 2001/08/08 00:12:37 eeh Exp $ */

/*
 * Copyright (c) 2001 Eduardo Horvath
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
 *      This product includes software developed by Eduardo Horvath.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"
#include <machine/asm.h>

/*
 * int in_cksum(struct mbuf *m, int len)
 *
 * The only fields of the mbuf we really care about
 * is m_next and m_len and m_data.
 *
 *
 * Register usage:
 *
 *	%o0 -	mbuf
 *	%o1 -	len
 *	%o2 -	mlen
 *	%o3 -	mdata
 *	%o4 -	temp
 *	%o5 -	sum
 *	%g1 -	swapped
 *	%g4 -	temp
 *	%g5 -	temp
 */

#define	IALIGN	.align	32
	
ENTRY(in_cksum)
	brz	%o0, Lfinish	! for (; m && len > 0; m->m_next) {
	 clr	%o5		! sum = 0;
	brlez	%o1, Lfinish
	 clr	%g1		! swapped = 0;
	ba,a,pt	%icc, Lloop
	 nop
	
	IALIGN
Lloop:			
	lduw	[%o0 + M_LEN], %o2
	sethi	%hi(0xffff), %g5
	ldx	[%o0 + M_DATA], %o3
	or	%g5, %lo(0xffff), %g5
	brz	%o2, Lnext	! if (m->m_len == 0) continue;
	
	 cmp	%o1, %o2	! if (len < mlen)
	movl	%icc, %o1, %o2	!	mlen = len;

	btst	3, %o3		! if (!(*w & 3)) {
	bz	Lint_aligned
	 sub	%o1, %o2, %o1	! len -= mlen

	srlx	%o5, 16, %o4	! REDUCE {sum = (sum & 0xffff) + (sum >> 16);}
	and	%o5, %g5, %o5
	
	add	%o5, %o4, %o5
	btst	1, %o3		! if (!(*w & 3) &&
	bz	Lshort_aligned
	 nop
	
	deccc	%o2
	bl,a,pn	%icc, Lfinish	! mlen >= 1) {
	 inc	%o2
	ldub	[%o3], %o4	! ADDBYTE {ROL; sum += *w; byte_swapped ^= 1;}
	sllx	%o5, 8, %o5	! ROL { sum = sum << 8; }
	inc	%o3		! }
	add	%o5, %o4, %o5
	xor	%g1, 1, %g1	! Flip byte_swapped
	
Lshort_aligned:
	btst	2, %o3		! if (!(*w & 3) &&
	bz	Lint_aligned
	 nop
	
	deccc	2, %o2		! mlen >= 1) {
	bl,a,pn	%icc, Lfinish_byte
	 inc	2, %o2
	lduh	[%o3], %o4	! ADDSHORT {sum += *(u_short *)w;}
	inc	2, %o3		! }
	add	%o5, %o4, %o5	! }
Lint_aligned:
	deccc	0xc, %o2	! while (mlen >= 12) {
	ble,pn	%icc, Ltoofar
	 clr	%g5
	ba,pt	%icc, 0f
	 clr	%g4
	IALIGN
0:	
	lduw	[%o3 + 0x00], %o4
	add	%o5, %g4, %o5
	deccc	0xc, %o2
	lduw	[%o3 + 0x04], %g4
	add	%o5, %g5, %o5
	lduw	[%o3 + 0x08], %g5
	inc	0xc, %o3	! ADVANCE(12) }
	bg,pt	%icc, 0b	
	 add	%o5, %o4, %o5
	add	%o5, %g4, %o5
	add	%o5, %g5, %o5
Ltoofar:
	inc	0xc, %o2
	
Ldo_int:
	deccc	4, %o2
	bl,pn	%icc, Lfinish_short
	 nop
0:	
	lduw	[%o3], %o4
	inc	4, %o3
	deccc	4, %o2
	bge,pt	%icc, 0b
	 add	%o5, %o4, %o5

Lfinish_short:	
	btst	2, %o2
	bz	Lfinish_byte
	 nop
	lduh	[%o3], %o4
	inc	2, %o3
	add	%o5, %o4, %o5

Lfinish_byte:	
	btst	1, %o2
	bz	Lnext
	 nop
	ldub	[%o3], %o4
	sllx	%o5, 8, %o5	! ROL { sum = sum << 8; }
	add	%o5, %o4, %o5
	inc	%o3
	xor	%g1, 1, %g1	! Flip byte_swapped
	
Lnext:
	ldx	[%o0 + M_NEXT], %o0
	brnz,pn	%o0, Lloop	! In general there is only one mbuf
	 nop
	brnz,pt	%o1, Lloop	! But usually all need to be fully checksummed
	 nop
Lfinish:
	srlx	%o5, 32, %o4	! REDUCE
	sethi	%hi(0x0000ffff), %o3	! data ptr not needed any more
	
	srl	%o5, 0, %o5	! Clear top 64 bits
	or	%o3, %lo(0x0000ffff), %o3
	
	add	%o4, %o5, %o5
	
	srl	%o5, 16, %o4
	and	%o5, %o3, %o5
	
	add	%o5, %o4, %o5
	brz,pt	%g1, 0f		! if (byte_swapped) {
	 nop

	sllx	%o5, 8, %o5	! ROL

	srl	%o5, 16, %o4	! REDUCE
	and	%o5, %o3, %o5
	
	add	%o5, %o4, %o5
0:	
	subcc	%o5, %o3, %o4	! if (sum > 0xffff)
	movg	%icc, %o4, %o5	! sum -= 0xffff;

	clr	%g4		! In case we are using EMBEDANY (ick)
	retl
	 xor	%o5, %o3, %o0	! return (0xffff ^ sum);
