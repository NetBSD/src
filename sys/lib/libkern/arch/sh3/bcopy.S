/*	$NetBSD: bcopy.S,v 1.1 2000/04/20 13:52:35 msaitoh Exp $	*/

/*
 * Copyright (c) 2000 SHIMIZU Ryo <ryo@misakimix.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#if defined(MEMCOPY) || defined(MEMMOVE)
#define	DST	r4
#define	SRC	r5
#else
#define	SRC	r4
#define	DST	r5
#endif
#define	LEN	r6


#ifdef MEMCOPY
ENTRY(memcpy)
#else
#ifdef MEMMOVE
ENTRY(memmove)
#else
ENTRY(bcopy)
#endif
#endif
	cmp/hi	DST,SRC
	bf	bcopy_overlap

	tst	LEN,LEN
	bt	9f
1:
	mov.b	@SRC+,r0
	mov.b	r0,@DST

	add	#-1,LEN
	tst	LEN,LEN
	bf/s	1b
	add	#1,DST

9:
	rts	
	nop

bcopy_overlap:
	add	LEN,SRC
	add	LEN,DST

	tst	LEN,LEN
	bt	9f
1:
	add	#-1,SRC
	mov.b	@SRC,r0

	add	#-1,LEN
	tst	LEN,LEN
	bf/s	1b
	mov.b	r0,@-DST

9:
	rts	
	nop

