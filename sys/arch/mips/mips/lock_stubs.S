/*	$NetBSD: lock_stubs.S,v 1.1.2.1 2007/01/11 22:22:57 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include "opt_cputype.h"

#include <machine/asm.h>
#include <machine/cpu.h>

#ifdef MIPS_HAS_LLSC

#ifdef MIPS64
#define	LL	lld
#define	SC	scd
#define	LDPTR	ld
#define	STPTR	sd
#else
#define	LL	ll
#define	SC	sc
#define	LDPTR	lw
#define	STPTR	sw
#endif

	.set	mips3
	.set	noreorder
	.set	noat

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
LEAF(_lock_cas)
1:	LL	t0, (a0)
	bne	t0, a1, 2f
	 add	t1, a2, a2
	SC	t1, (a0)
	beq	t1, zero, 1b
	 nop
	j	ra
	 li	v0, 1

2:	j	ra
	 add	v0, zero, zero
END(_lock_cas)

#ifndef LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
LEAF(mutex_enter)
1:	LL	t0, (a0)
	LDPTR	t1, _C_LABEL(curlwp)
	bne	t0, zero, 2f
	 nop
	SC	t1, (a0)
	beq	t1, zero, 1b
	 nop
	j	ra
	 nop

2:	j	_C_LABEL(mutex_vector_enter)
	 nop
END(mutex_enter)

/*
 * void	mutex_exit(kmutex_t *mtx);
 */
LEAF(mutex_exit)
1:	LL	t0, (a0)
	LDPTR	t1, _C_LABEL(curlwp)
	nop
	bne	t0, t1, 2f
	add	t1, zero, zero
	SC	t1, (a0)
	beq	t1, zero, 1b
	 nop
	j	ra
	 nop

2:	j	_C_LABEL(mutex_vector_exit)
	 nop
END(mutex_exit)

#endif	/* !LOCKDEBUG */

#else	/* MIPS_HAS_LLSC */

/*
 * Lock stubs for MIPS1 and a couple of oddball MIPS3 CPUs.  These are
 * implemented using restartable sequences, since LL/SC are not available.
 *
 * The order of the generated code is particularly important here..  Some
 * assmptions:
 *
 * o We won't reference more than one memory location in KSEG2 during
 *   a critical section, so that there's reduced chance of the TLB miss
 *   handler us bouncing back around for a second go.
 *
 * o All of the critical sections are 20 bytes in size, and the entry
 *   to each critical section is aligned on a 16 byte boundary (see
 *   top of _lock_ras() for why).
 *
 * o The entire code block is aligned on a 128 byte boundary, and is
 *   128 bytes in size.  This is to allow us to do an pessimistic check
 *   after taking a trap with:
 *
 *	if ((addr & ~127) == _lock_ras_start)
 *		addr = _lock_ras(addr);
 *
 * o In order to keep the size of the block down, the routines are run
 *   into each other.  Use objdump -d to check alignment after making
 *   changes.
 */
	.set	mips1
	.set	noreorder
	.set	noat

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
	.align	7
EXPORT(_lock_ras_start)
LEAF(_lock_cas)
_lock_cas_start:
	LDPTR	t0, (a0)	/* <- critical section start */
	 nop
	bne	t0, a1, 1f
	 nop
	STPTR	a2, (a0)	/* <- critical section end */
	j	ra
	 li	v0, 1
1:	j	ra
	 add	v0, zero, zero
END(_lock_cas)

#ifndef LOCKDEBUG
/*
 * int mutex_enter(kmutex_t *mtx);
 */
	nop
	nop
LEAF(mutex_enter)
	lui	t2, %hi(_C_LABEL(curlwp))
_mutex_enter_start:
	LDPTR	t0, (a0)	/* <- critical section start */
	LDPTR	t1, (%lo(_C_LABEL(curlwp)))(t2)
	bne	t0, zero, 1f
	 nop			
	STPTR	t1, (a0)	/* <- critical section end */
	j	ra
	 nop
1:	j	_C_LABEL(mutex_vector_enter)
	 nop
END(mutex_enter)

/*
 * int mutex_exit(kmutex_t *mtx);
 */
	nop
LEAF(mutex_exit)
	lui	t2, %hi(_C_LABEL(curlwp))
	LDPTR	t1, (%lo(_C_LABEL(curlwp)))(t2)
_mutex_exit_start:
	LDPTR	t0, (a0)	/* <- critical section start */
	 nop
	bne	t0, t1, 1f
	 nop			
	STPTR	zero, (a0)	/* <- critical section end */
	j	ra
	 nop
1:	j	_C_LABEL(mutex_vector_exit)
EXPORT(_lock_ras_end)
	 nop
END(mutex_exit)

#else	/* !LOCKDEBUG */

/*
 * _lock_cas() is 32 bytes, so add another 96.
 */
	.space	0x60
EXPORT(_lock_ras_end)

#endif	/* !LOCKDEBUG */

/*
 * Patch up the given address.  We arrive here if we might have trapped
 * within one of the critical sections above.  Do:
 *
 *	if ((addr & ~15) == ras)
 *		return ras;
 *	if ((addr - 16) == ras)
 *		return ras;
 *	... check next ...
 *	return addr;
 *
 * Registers on entry:
 *
 *	k1	fault PC
 *	ra	return address
 *
 * On exit:
 *
 *	k1	adjusted fault PC
 *	ra	return address
 *	t0	clobbered
 *	t1	clobbered
 *	t2	clobbered
 */
LEAF(_lock_ras)
	addi	t1, zero, -16

#ifndef LOCKDEBUG
	lui	t0, %hi($_mutex_enter_start)
	ori	t0, t0, %lo($_mutex_enter_start)
	and	t2, k1, t1
	beq	t2, t0, 1f
	 nop
	addu	t2, t2, t1
	beq	t2, t0, 1f
	
	 lui	t0, %hi($_mutex_exit_start)
	ori	t0, t0, %lo($_mutex_exit_start)
	and	t2, k1, t1
	beq	t2, t0, 1f
	 nop
	addu	t2, t2, t1 
	beq	t2, t0, 1f
#endif	/* !LOCKDEBUG */

	 lui	t0, %hi($_lock_cas_start)
	ori	t0, t0, %lo($_lock_cas_start)
	and	t2, k1, t1
	beq	t2, t0, 1f
	 nop
	addu	t2, t2, t1 
	beq	t2, t0, 1f
	 nop

	j	ra
	 nop

1:	j	ra
	 add	k1, zero, t2
END(_lock_ras)

#endif	/* MIPS_HAS_LLSC */
