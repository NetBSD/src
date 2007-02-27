/*	$NetBSD: lock_stubs.S,v 1.2.6.2 2007/02/27 16:53:00 yamt Exp $	*/

/*
 * Copyright (c) 2007 Valeriy E. Ushakov
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

#include <sh3/asm.h>
#include <sh3/psl.h>

/*
 * LINTSTUB: include <sys/types.h>
 */


/*
 * LINTSTUB: Func: int _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 *	Brute force with splhigh() for now.  Need to be redone as RAS.
 */
NENTRY(_lock_cas)
	mov	#PSL_IMASK, r2	! 0xf0 - gets sign extended
	stc	sr, r3
	extu.b	r2, r2		! undo sign extention
	mov	r3, r7		! preserve SR
	or	r2, r3		! mask all interrupts
	ldc	r3, sr

	mov.l	@r4, r1
	cmp/eq	r1, r5		! T = (*ptr == old)
	bf.s	1f
	 movt	r0		! retval = T
	mov.l	r6, @r4		! *ptr = new
1:
	ldc	r7, sr		! restore SR (cannot be in delay slot)
	rts
	 nop
