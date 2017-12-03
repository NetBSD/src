/*	$NetBSD: lock_stubs_ras.S,v 1.4.12.1 2017/12/03 11:36:28 jdolecek Exp $	*/

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
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <sys/errno.h>

#include <machine/asm.h>

RCSID("$NetBSD: lock_stubs_ras.S,v 1.4.12.1 2017/12/03 11:36:28 jdolecek Exp $")

#include "assym.h"

/*
 * We rely on mips_vector_init to choose to not use these routines if we are
 * on a system with multiple CPUs.  We can still use this on a simple CPU
 * with MULTIPROCESSOR since it might be useful to be using preemption.
 */

/*
 * Lock stubs for non-MP kernels.  These are implemented using restartable
 * sequences, since LL/SC are either not available (MIPS1 and a couple of
 * oddball MIPS3 CPUs) or not desirable (overhead).
 *
 * The order of the generated code is particularly important here.  Some
 * assumptions:
 *
 * o All of the critical sections are 20 bytes in size, and the second
 *   instruction in each critical section is aligned on a 16 byte boundary
 *   (see top of _restart_lock_ras() for why).  The entry is defined here as
 *   the point where a restart occurs if we trap within the section.
 *
 * o The entire code block is aligned on a 256 byte boundary, and is
 *   256 bytes in size.  This is to allow us to do an pessimistic check
 *   after taking a trap with:
 *
 *	if ((addr & ~255) == _lock_ras_start)
 *		addr = _restart_lock_ras(addr);
 *
 *   See definition of MIPS_LOCK_RAS_SIZE in asm.h.
 *
 * o In order to keep the size of the block down, the routines are run
 *   into each other.  Use objdump -d to check alignment after making
 *   changes.
 */
#ifndef __mips_o32
	.set	mips3
#else
	.set	mips1
#endif
	.set	noreorder
	.set	noat

/*
 * to work around the branch prediction engine misbehavior of
 * Loongson 2F processors we need to clear the branch target buffer before
 * a j ra.  This requires extra instructions which don't fit in the RAS blocks,
 * so do a PC-relative just to a block of code (this is the same size as
 * a j ra) where we can let the assembler install the workaround.
 */
#ifdef MIPS3_LOONGSON2F
#define J_RA	j loongson_return
#else
#define J_RA	j ra
#endif


/*
 * unsigned long ras_atomic_cas_ulong(volatile unsigned long *val,
 *     unsigned long old, unsigned long new);
 */
	.text
	.p2align LOG2_MIPS_LOCK_RAS_SIZE

EXPORT(_lock_ras_start)
STATIC_LEAF_NOPROFILE(ras_atomic_cas_noupdate)
	J_RA
	 move	v0, t0
END(ras_atomic_cas_noupdate)

	nop
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bas ras offset"
	.endif
STATIC_LEAF_NOPROFILE(ras_atomic_cas_ulong)
	PTR_L	t0, (a0)	/* <- critical section start */
_atomic_cas_ulong_ras_start:
	 nop
	bne	t0, a1, ras_atomic_cas_noupdate
 	 nop
	PTR_S	a2, (a0)	/* <- critical section end */
_atomic_cas_ulong_ras_end:
	J_RA
	 move	v0, a1
END(ras_atomic_cas_ulong)

/*
 * unsigned int ras_atomic_cas_uint(volatile unsigned int *val,
 *     unsigned int old, unsigned int new);
 */
	nop
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bas ras offset"
	.endif
STATIC_LEAF_NOPROFILE(ras_atomic_cas_uint)
	INT_L	t0, (a0)	/* <- critical section start */
_atomic_cas_uint_ras_start:
	 nop
	bne	t0, a1, ras_atomic_cas_noupdate
 	 nop
	INT_S	a2, (a0)	/* <- critical section end */
_atomic_cas_uint_ras_end:
	J_RA
	 move	v0, a1
END(ras_atomic_cas_uint)

/*
 * int _ucas_ulong_ras(volatile u_long *val, u_long old, u_long new,
 *     u_long *retp);
 */
	nop
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bas ras offset"
	.endif
STATIC_LEAF_NOPROFILE(_ucas_ulong_ras)
	LONG_L	t0, (a0)	/* <- critical section start */
_ucas_ulong_ras_start:
	 nop
	bne	t0, a1, _ucas_ulong_ras_end
 	 nop
	LONG_S	a2, (a0)	/* <- critical section end */
_ucas_ulong_ras_end:
	PTR_S	zero, PCB_ONFAULT(v1)
	J_RA
	 LONG_S	t0, 0(a3)
END(_ucas_ulong_ras)

/*
 * int _ucas_uint_ras(volatile u_int *val, u_int old, u_int new, u_int *retp);
 */
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bad ras offset"
	.endif
STATIC_LEAF_NOPROFILE(_ucas_uint_ras)
	INT_L	t0, (a0)	/* <- critical section start */
_ucas_uint_ras_start:
	 nop
	bne	t0, a1, _ucas_uint_ras_end
 	 nop
	INT_S	a2, (a0)	/* <- critical section end */
_ucas_uint_ras_end:
	PTR_S	zero, PCB_ONFAULT(v1)
	J_RA
	 INT_S	t0, 0(a3)
END(_ucas_uint_ras)

#ifndef LOCKDEBUG
/*
 * void ras_mutex_enter(kmutex_t *mtx);
 */
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bad ras offset"
	.endif
STATIC_LEAF_NOPROFILE(ras_mutex_enter)
	PTR_L	t0, (a0)	/* <- critical section start */
_mutex_enter_ras_start:
	 nop
	bnez	t0, ras_mutex_vector_enter
	 nop
	PTR_S	MIPS_CURLWP, (a0)/* <- critical section end */
_mutex_enter_ras_end:
	J_RA
	 nop
END(ras_mutex_enter)

/*
 * int ras_mutex_exit(kmutex_t *mtx);
 */
	nop
	.if ((. - _lock_ras_start) & 15) != 12
	.error	"bas ras offset"
	.endif
STATIC_LEAF_NOPROFILE(ras_mutex_exit)
	PTR_L	t0, (a0)	/* <- critical section start */
_mutex_exit_ras_start:
	 nop
	bne	t0, MIPS_CURLWP, ras_mutex_vector_exit
	 nop
	PTR_S	zero, (a0)	/* <- critical section end */
_mutex_exit_ras_exit:
	J_RA
	 nop
END(ras_mutex_exit)

/*
 * These could moved out to fit in more RAS sequences.
 */
STATIC_LEAF_NOPROFILE(ras_mutex_vector_enter)
	j	_C_LABEL(mutex_vector_enter)
	 nop
END(ras_mutex_vector_enter)

STATIC_LEAF_NOPROFILE(ras_mutex_vector_exit)
	j	_C_LABEL(mutex_vector_exit)
	 nop
END(ras_mutex_vector_exit)
#endif	/* !LOCKDEBUG */

	.p2align LOG2_MIPS_LOCK_RAS_SIZE	/* Get out of the RAS block */

	.set at
#ifdef MIPS3_LOONGSON2F
loongson_return:
	j	ra
	 nop
#endif

/*
 * Patch up the given address.  We arrive here if we might have trapped
 * within one of the critical sections above.  Do:
 *
 *	if ((addr & ~15) == ras)
 *		return ras - 4;
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
 */

#define	RAS_MKMASK(a)	(1 << (((a)-_lock_ras_start) >> 4))

/*
 * Since each RAS is aligned on a 16 byte boundary, we can use its offset
 * from _lock_ras_start to construct a bitmask of the valid RAS within.
 */
#ifndef LOCKDEBUG
#define	MUTEX_RAS_MASK	(RAS_MKMASK(_mutex_enter_ras_start) \
			|RAS_MKMASK(_mutex_exit_ras_start))
#else
#define	MUTEX_RAS_MASK	0
#endif

#define	RAS_MASK	(RAS_MKMASK(_atomic_cas_ulong_ras_start) \
			|RAS_MKMASK(_atomic_cas_uint_ras_start) \
			|RAS_MKMASK(_ucas_ulong_ras_start) \
			|RAS_MKMASK(_ucas_uint_ras_start) \
			|MUTEX_RAS_MASK)

/*
 * The caller has already determined that
 * _lock_ras_start == (k1 & -MIPS_LOCK_RAS_SIZE)
 */
LEAF_NOPROFILE(_restart_lock_ras)
	and	t0, k1, MIPS_LOCK_RAS_SIZE - 1
				/* look at addr bits in ras region */
	srl	t0, 4		/* focus on each set of 16 bytes */
	li	t1, 1		/* need this to make a bitmask */
	sllv	t1, t1, t0	/* now we have a bitmask of the PC */
	andi	t1, RAS_MASK	/* was the PC in a RAS? */
	bnez	t1, 1f		/* yes, adjust PC */
	 and	t0, k1, 15	/* get offset in RAS */

	j	ra
	 nop
1:
	addu	t0, 4		/* bias offset by one more instruction */
	j	ra
	 PTR_SUBU k1, t0	/* and subtract that from the PC */
END(_restart_lock_ras)

/*
 * int ras_ucas_uint(volatile u_int *ptr, u_int old, u_int new, u_int *retp);
 */
STATIC_LEAF(ras_ucas_uint)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(ras_ucaserr)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(ras_ucaserr)
	 nop
	b	_C_LABEL(_ucas_uint_ras)
	 move	v0, zero			# assume success
END(ras_ucas_uint)

/*
 * int ras_ucas_ulong(volatile u_long *ptr, u_long old, u_long new, u_long *retp);
 */
STATIC_LEAF(ras_ucas_ulong)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(ras_ucaserr)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(ras_ucaserr)
	 nop
	b	_C_LABEL(_ucas_ulong_ras)
	 move	v0, zero			# assume success
END(ras_ucas_ulong)

/*
 *
 */
STATIC_LEAF(ras_ucaserr)
	PTR_S	zero, PCB_ONFAULT(v1)		# reset fault handler
	j	ra
	 li	v0, EFAULT			# return EFAULT on error
END(ras_ucaserr)

#ifndef LOCKDEBUG
/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
STATIC_NESTED(ras_mutex_spin_enter, CALLFRAME_SIZ, ra)
	move	t0, a0
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	INT_L	a0, MTX_IPL(t0)
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_CPL(t2)		# get current cpl
#endif

	/*
	 * We need to raise our IPL.
	 * call splraise (only uses a0-a3, v0-v1, and ra)
	 */
	move	t3, ra
	jal	_C_LABEL(splraise)
	 nop
	move	ra, t3

	/*
	 * If this is the first lock of the mutex, store the previous IPL
	 * for exit.
	 */
1:
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	nop
	INT_ADDU ta3, ta2, -1
	INT_S	ta3, CPU_INFO_MTX_COUNT(t2)

	bnez	ta2, 2f
	 nop
	INT_S	v0, CPU_INFO_MTX_OLDSPL(t2)	/* returned by splraise */
2:
#if defined(DIAGNOSTIC)
	INT_L	t3, MTX_LOCK(t0)
	li	t1, 1
	bnez	t3, 3f
	 nop
	j	ra
	 INT_S	t1, MTX_LOCK(t0)
3:
	j	_C_LABEL(mutex_spin_retry)
	 nop
#else	/* DIAGNOSTIC */
	j	ra
	 nop
#endif	/* DIAGNOSTIC */
END(ras_mutex_spin_enter)

/*
 * void	mutex_spin_exit(kmutex_t *mtx);
 */
LEAF(ras_mutex_spin_exit)
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	nop
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	nop
	beqz	t0, 2f
	 nop
	INT_S	zero, MTX_LOCK(a0)
#endif

	/*
	 * We need to grab this before the mutex count is incremented
	 * because if we get an interrupt, it may see the count as zero
	 * and overwrite the oldspl value with a bogus value.
	 */
#ifdef PARANOIA
	INT_L	a2, MTX_IPL(a0)
#endif
	INT_L	a0, CPU_INFO_MTX_OLDSPL(t2)

	/*
	 * Increment the mutex count
	 */
	INT_L	t0, CPU_INFO_MTX_COUNT(t2)
	nop
	INT_ADDU t0, t0, 1
	INT_S	t0, CPU_INFO_MTX_COUNT(t2)

	/*
	 * If the IPL doesn't change, nothing to do
	 */
	INT_L	a1, CPU_INFO_CPL(t2)
	nop

#ifdef PARANOIA
	sltu	v0, a1, a2		# v0 = cpl < mtx_ipl
	sltu	v1, a1, a0		# v1 = cpl < oldspl
	sll	v0, 1
	or	v0, v1
12:	bnez	v0, 12b			# loop forever if either is true
	 nop
#endif /* PARANOIA */

	beq	a0, a1, 1f		# if oldspl == cpl
	 nop				#   no reason to drop ipl

	bltz	t0, 1f			# there are still holders
	 nop				# so don't drop IPL

	/*
	 * Mutex count is zero so we need to restore the old IPL
	 */
#ifdef PARANOIA
	sltiu	v0, a0, IPL_HIGH+1
13:	beqz	v0, 13b			# loop forever if ipl > IPL_HIGH
	 nop
#endif
	j	 _C_LABEL(splx)
	 nop
1:
	j	ra
	 nop
#if defined(DIAGNOSTIC)
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
#endif
END(ras_mutex_spin_exit)
#endif	/* !LOCKDEBUG */

	.data
EXPORT(mips_locore_atomicvec)
	PTR_WORD 	ras_atomic_cas_uint
	PTR_WORD 	ras_atomic_cas_ulong
	PTR_WORD	ras_ucas_uint
	PTR_WORD	ras_ucas_ulong
#ifdef LOCKDEBUG
	PTR_WORD	mutex_enter
	PTR_WORD	mutex_exit
	PTR_WORD	mutex_spin_enter
	PTR_WORD	mutex_spin_exit
#else
	PTR_WORD	ras_mutex_enter
	PTR_WORD	ras_mutex_exit
	PTR_WORD	ras_mutex_spin_enter
	PTR_WORD	ras_mutex_spin_exit
#endif	/* !LOCKDEBUG */
