/*	$NetBSD: macros.h,v 1.15 1998/03/02 17:00:01 ragge Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
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

 /* All bugs are subject to removal without further notice */

#if !defined(_VAX_MACROS_H_) && !defined(STANDALONE) && \
	(!defined(_LOCORE) && defined(_VAX_INLINE_))
#define _VAX_MACROS_H_

/* Here general macros are supposed to be stored */

static __inline__ int ffs(int reg){
	register int val;

	__asm__ __volatile ("ffs	$0,$32,%1,%0
			bneq	1f
			mnegl	$1,%0
		1:	incl	%0"
			: "&=r" (val)
			: "r" (reg) );
	return	val;
}

static __inline__ void _remque(void*p){
	__asm__ __volatile ("remque (%0),%0;clrl 4(%0)"
			:
			: "r" (p)
			: "memory" );
}

static __inline__ void _insque(void*p, void*q) {
	__asm__ __volatile ("insque (%0), (%1)"
			:
			: "r" (p),"r" (q)
			: "memory" );
}

static __inline__ void bcopy(const void*from, void*toe, u_int len) {
	__asm__ __volatile ("movc3 %0,(%1),(%2)"
			:
			: "r" (len),"r" (from),"r"(toe)
			:"r0","r1","r2","r3","r4","r5");
}

void	blkclr __P((void *, u_int));

static __inline__ void bzero(void*block, u_int len){
	if (len > 65535)
		blkclr(block, len);
	else {
		__asm__ __volatile ("movc5 $0,(%0),$0,%1,(%0)"
			:
			: "r" (block), "r" (len)
			:"r0","r1","r2","r3","r4","r5");
	}
}

static __inline__ int bcmp(const void *b1, const void *b2, size_t len){
	register ret;

	__asm__ __volatile("cmpc3 %3,(%1),(%2);movl r0,%0"
			: "=r" (ret)
			: "r" (b1), "r" (b2), "r" (len)
			: "r0","r1","r2","r3" );
	return ret;
}

#if 0 /* unused, but no point in deleting it since it _is_ an instruction */
static __inline__ int locc(int mask, char *cp,u_int size){
	register ret;

	__asm__ __volatile("locc %1,%2,(%3);movl r0,%0"
			: "=r" (ret)
			: "r" (mask),"r"(size),"r"(cp)
			: "r0","r1" );
	return	ret;
}
#endif

static __inline__ int
scanc(u_int size, const u_char *cp, const u_char *table, int mask){
	register ret;

	__asm__ __volatile("scanc	%1,(%2),(%3),%4;movl r0,%0"
			: "=g"(ret)
			: "r"(size),"r"(cp),"r"(table),"r"(mask)
			: "r0","r1","r2","r3" );
	return ret;
}

static __inline__ int skpc(int mask, size_t size, u_char *cp){
	register ret;

	__asm__ __volatile("skpc %1,%2,(%3);movl r0,%0"
			: "=g"(ret)
			: "r"(mask),"r"(size),"r"(cp)
			: "r0","r1" );
	return	ret;
}

#define setrunqueue(p)	\
	__asm__ __volatile("movl %0,r0;jsb Setrq":: "g"(p):"r0","r1","r2");

#define remrunqueue(p)	\
	__asm__ __volatile("movl %0,r0;jsb Remrq":: "g"(p):"r0","r1","r2");

#define cpu_switch(p) \
	__asm__ __volatile("movl %0,r0;movpsl -(sp);jsb Swtch" \
	    ::"g"(p):"r0","r1","r2","r3");
#endif	/* _VAX_MACROS_H_ */
