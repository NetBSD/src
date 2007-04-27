/*	$NetBSD: atomic_cas.S,v 1.1.2.1 2007/04/27 05:57:38 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atomic_op_asm.h"

#if defined(_KERNEL)

#include "opt_multiprocessor.h"

#define	DISABLE_INTERRUPTS						 \
	rd	%psr, %o4			! disable interrupts	;\
	or	%o4, PSR_IPL, %o5					;\
	wr	%o5, 0, %psr						;\
	nop								;\
	nop								;\
	nop

#define	RESTORE_INTERRUPTS						 \
	wr	%o4, 0, %psr			! re-enable interrupts	;\
	nop								;\
	nop								;\
	nop

#if defined(MULTIPROCESSOR)
	.section .bss
	.align	1024
OTYPE(_C_LABEL(_atomic_cas_locktab))
_C_LABEL(_atomic_cas_locktab):
	.space	1024

#define	ACQUIRE_INTERLOCK						 \
	DISABLE_INTERRUPTS						;\
	srl	%o0, 3, %o5			! get interlock address	;\
	and	%o5, 1023, %o5						;\
	set	_C_LABEL(_atomic_cas_locktab), %o3			;\
	add	%o5, %o3, %o5						;\
									;\
	! %o5 has interlock address					;\
									;\
1:	ldstub	[%o5], %o3			! acquire interlock	;\
	tst	%o3							;\
	bz,a	2f							;\
	 nop								;\
	nop								;\
	nop								;\
	b,a	1b				! spin			;\
	 nop								;\
									;\
	! We now hold the interlock					;\
2:

#define	RELEASE_INTERLOCK						 \
	stb	%g0, [%o5]			! release interlock	;\
	RESTORE_INTERRUPTS

#else /* ! MULTIPROCESSOR */

#define	ACQUIRE_INTERLOCK	DISABLE_INTERRUPTS

#define	RELEASE_INTERLOCK	RESTORE_INTERRUPTS

#endif /* MULTIPROCESSOR */

	.text

/*
 * The v7 and v8 SPARC doesn't have compare-and-swap, so we block interrupts
 * and use an interlock.
 *
 * XXX On single CPU systems, this should use a restartable sequence:
 * XXX there we don't need the overhead of interlocking.
 *
 * XXX NOTE!  The interlock trick only works if EVERYTHING writes to
 * XXX the memory cell through this code path!
 *
 * XXX NOTE!  We rely on 8-bit and 16-bit ops to resolve to the same
 * XXX interlock as their corresponding 32-bit word.
 */
ENTRY_NOPROFILE(_atomic_cas_8)
	ACQUIRE_INTERLOCK
	! %o4 has saved PSR value
	! %o5 has interlock address

	ldub	[%o0], %o3			! get old value
	cmp	%o1, %o3			! old == new?
	beq,a	3f				! yes, do the store
	 stb	%o2, [%o0]			! (in the delay slot)

3:	RELEASE_INTERLOCK

	retl
	 mov	%o3, %o0			! return old value

ATOMIC_OP_ALIAS(atomic_cas_8,_atomic_cas_8)
ATOMIC_OP_ALIAS(atomic_cas_uchar,_atomic_cas_8)
STRONG_ALIAS(_atomic_cas_uchar,_atomic_cas_8)

ENTRY_NOPROFILE(_atomic_cas_16)
	ACQUIRE_INTERLOCK
	! %o4 has saved PSR value
	! %o5 has interlock address

	lduh	[%o0], %o3			! get old value
	cmp	%o1, %o3			! old == new?
	beq,a	3f				! yes, do the store
	 sth	%o2, [%o0]			! (in the delay slot)

3:	RELEASE_INTERLOCK

	retl
	 mov	%o3, %o0			! return old value

ATOMIC_OP_ALIAS(atomic_cas_16,_atomic_cas_16)
ATOMIC_OP_ALIAS(atomic_cas_ushort,_atomic_cas_16)
STRONG_ALIAS(_atomic_cas_ushort,_atomic_cas_16)

ENTRY_NOPROFILE(_atomic_cas_32)
	ACQUIRE_INTERLOCK
	! %o4 has saved PSR value
	! %o5 has interlock address

	ld	[%o0], %o3			! get old value
	cmp	%o1, %o3			! old == new?
	beq,a	3f				! yes, do the store
	 st	%o2, [%o0]			! (in the delay slot)

3:	RELEASE_INTERLOCK

	retl
	 mov	%o3, %o0			! return old value

ATOMIC_OP_ALIAS(atomic_cas_32,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32)

#else /* _KERNEL */

#endif /* _KERNEL */
