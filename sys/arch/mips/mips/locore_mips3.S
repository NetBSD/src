/*	$NetBSD: locore_mips3.S,v 1.95 2009/12/10 05:10:02 rmind Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone (hereinafter referred to as the author)
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
 *      This product includes software developed by Jonathan R. Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
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
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */
#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <machine/param.h>
#include <machine/endian.h>

#include "assym.h"

/*
 * XXX We need a cleaner way of handling the instruction hazards of
 * the various processors.  Here are the relevant rules for the QED 52XX:
 *	tlbw[ri]	-- two integer ops beforehand
 *	tlbr		-- two integer ops beforehand
 *	tlbp		-- two integer ops beforehand
 *	mtc0	[PageMask,EntryHi,Cp0] -- two integer ops afterwards
 *	changing JTLB	-- two integer ops afterwards
 *	mtc0	[EPC,ErrorEPC,Status] -- two int ops afterwards before eret
 *	config.k0	-- five int ops before kseg0, ckseg0 memref
 *
 * For the IDT R4000, some hazards are:
 *	mtc0/mfc0	one integer op before and after
 *	tlbp		-- one integer op afterwards
 * Obvious solution is to take least common denominator.
 *
 * For the Toshiba R5900, TX79:
 *	mtc0		following sync.p
 *	tlbw[ri], tlbp	following sync.p or eret
 * for those CPU, define COP0_SYNC as sync.p
 */

/*
 *============================================================================
 *
 *  MIPS III ISA support, part 1: locore exception vectors.
 *  The following code is copied to the vector locations to which
 *  the CPU jumps in response to an exception or a TLB miss.
 *
 *============================================================================
 */
	.set	noreorder
	.set	mips3

	.text

/*----------------------------------------------------------------------------
 *
 * mips3_wbflush --
 *
 *	Return when the write buffer is empty.
 *
 *	Common for all MIPS3 and greater ISAs
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_wbflush)
XLEAF(mips5900_wbflush)
XLEAF(mips32_wbflush)
XLEAF(mips64_wbflush)
	nop
	sync
	j	ra
	nop
END(mips3_wbflush)


/* XXX simonb: ugg, another ugly #ifdef check... */
#if (defined(MIPS3) && !defined(MIPS3_5900)) || defined(MIPS32) || defined(MIPS64)
/*
 * mips_wait_idle:
 *
 *	When no processes are on the runq, cpu_idle branches to
 *	mips_wait_idle to save power.
 */
LEAF(mips_wait_idle)
	wait
	nop
	nop
	nop
	j	ra
	nop
END(mips_wait_idle)
#endif /* (MIPS3 && !MIPS3_5900) || MIPS32 || MIPS64 */

/*
 * u_int32_t mips3_cp0_compare_read(void)
 *
 *	Return the current value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_read)
	mfc0	v0, MIPS_COP_0_COMPARE
	j	ra
	nop
END(mips3_cp0_compare_read)

/*
 * void mips3_cp0_compare_write(u_int32_t)
 *
 *	Set the value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_write)
	mtc0	a0, MIPS_COP_0_COMPARE
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips3_cp0_compare_write)

/*
 * u_int32_t mips3_cp0_config_read(void)
 *
 *	Return the current value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_read)
	mfc0	v0, MIPS_COP_0_CONFIG
	j	ra
	nop
END(mips3_cp0_config_read)

/*
 * void mips3_cp0_config_write(u_int32_t)
 *
 *	Set the value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_write)
	mtc0	a0, MIPS_COP_0_CONFIG
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips3_cp0_config_write)

#if defined(MIPS32) || defined(MIPS64)
/*
 * u_int32_t mipsNN_cp0_config1_read(void)
 *
 *	Return the current value of the CP0 Config (Select 1) register.
 */
LEAF(mipsNN_cp0_config1_read)
	.set push
	.set mips32
	mfc0	v0, MIPS_COP_0_CONFIG, 1
	j	ra
	nop
	.set pop
END(mipsNN_cp0_config1_read)

/*
 * u_int32_t mipsNN_cp0_config1_write(u_int32_t)
 *
 *	Return the current value of the CP0 Config (Select 1) register.
 */
LEAF(mipsNN_cp0_config1_write)
	.set push
	.set mips32
	mtc0	v0, MIPS_COP_0_CONFIG, 1
	nop
	j	ra
	nop
	.set pop
END(mipsNN_cp0_config1_write)

/*
 * u_int32_t mipsNN_cp0_config2_read(void)
 *
 *	Return the current value of the CP0 Config (Select 2) register.
 */
LEAF(mipsNN_cp0_config2_read)
	.set push
	.set mips32
	mfc0	v0, MIPS_COP_0_CONFIG, 2
	j	ra
	nop
	.set pop
END(mipsNN_cp0_config2_read)

/*
 * u_int32_t mipsNN_cp0_config3_read(void)
 *
 *	Return the current value of the CP0 Config (Select 3) register.
 */
LEAF(mipsNN_cp0_config3_read)
	.set push
	.set mips32
	mfc0	v0, MIPS_COP_0_CONFIG, 3
	j	ra
	nop
	.set pop
END(mipsNN_cp0_config3_read)
#endif /* MIPS32 || MIPS64 */

/*
 * u_int32_t mips3_cp0_count_read(void)
 *
 *	Return the current value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_read)
	mfc0	v0, MIPS_COP_0_COUNT
	j	ra
	nop
END(mips3_cp0_count_read)

/*
 * void mips3_cp0_count_write(u_int32_t)
 *
 *	Set the value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_write)
	mtc0	a0, MIPS_COP_0_COUNT
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips3_cp0_count_write)

/*
 * u_int32_t mips3_cp0_wired_read(void)
 *
 *	Return the current value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_read)
	mfc0	v0, MIPS_COP_0_TLB_WIRED
	j	ra
	nop
END(mips3_cp0_wired_read)

/*
 * void mips3_cp0_wired_write(u_int32_t)
 *
 *	Set the value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_write)
	mtc0	a0, MIPS_COP_0_TLB_WIRED
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips3_cp0_wired_write)

/*
 * void mips3_cp0_pg_mask_write(u_int32_t)
 *
 *	Set the value of the CP0 PG_MASK register.
 */
LEAF(mips3_cp0_pg_mask_write)
	mtc0	a0, MIPS_COP_0_TLB_PG_MASK
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips3_cp0_pg_mask_write)

#if defined(_MIPS_BSD_API) && \
    (_MIPS_BSD_API == _MIPS_BSD_API_N32 || _MIPS_BSD_API == _MIPS_BSD_API_LP64)
#error mips3_ld and mips3_sd should be adjusted for N32 or LP64
#endif

LEAF(mips3_ld)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mfc0	t0, MIPS_COP_0_STATUS		# turn off interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	nop
#endif

	ld	v0, 0(a0)
#if _BYTE_ORDER == _BIG_ENDIAN
	dsll	v1, v0, 32
	dsra	v1, v1, 32			# low word in v1
	dsra	v0, v0, 32			# high word in v0
#else
	dsra	v1, v0, 32			# high word in v1
	dsll	v0, v0, 32
	dsra	v0, v0, 32			# low word in v0
#endif

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	COP0_SYNC
	nop
#endif

	jr	ra
	nop
END(mips3_ld)

LEAF(mips3_sd)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mfc0	t0, MIPS_COP_0_STATUS		# turn off interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	nop
#endif

	# NOTE: a1 is padding!

#if _BYTE_ORDER == _BIG_ENDIAN
	dsll	a2, a2, 32			# high word in a2
	dsll	a3, a3, 32			# low word in a3
	dsrl	a3, a3, 32
#else
	dsll	a2, a2, 32			# low word in a2
	dsrl	a2, a2, 32
	dsll	a3, a3, 32			# high word in a3
#endif
	or	a1, a2, a3
	sd	a1, 0(a0)

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	COP0_SYNC
	nop
#endif

	jr	ra
	nop
END(mips3_sd)

/*
 * int badaddr64(uint64_t addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access in bytes (can be 1, 2, 4, or 8).
 */
#if defined(_MIPS_BSD_API) && _MIPS_BSD_API != _MIPS_BSD_API_LP32
LEAF(badaddr64)
	lw	v1, L_PCB(MIPS_CURLWP)
	la	v0, _C_LABEL(baderr64)

	/* Enable KX */
	mfc0	t0, MIPS_COP_0_STATUS
	or	t1, t0, MIPS3_SR_KX
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC

#if _BYTE_ORDER == _BIG_ENDIAN
	dsll	a0, a0, 32			# high word in a0
	dsll	a1, a1, 32			# low word in a1
	dsrl	a1, a1, 32
#else
	dsll	a0, a0, 32			# low word in a2
	dsrl	a0, a0, 32
	dsll	a1, a1, 32			# high word in a3
#endif
	or	a0, a0, a1

	bne	a2, 1, 2f
	sw	v0, PCB_ONFAULT(v1)
	b	9f
	lbu	v0, (a0)
2:
	bne	a2, 2, 4f
	nop
	b	9f
	lhu	v0, (a0)
4:
	bne	a2, 4, 8f
	nop
	b	9f
	lw	v0, (a0)
8:
	ld	v0, (a0)
9:
	mtc0	t0, MIPS_COP_0_STATUS           # Restore KX
	COP0_SYNC
	sw	zero, PCB_ONFAULT(v1)
	j	ra
	move	v0, zero			# made it w/o errors
END(badaddr64)

LEAF(baderr64)
	mtc0	t0, MIPS_COP_0_STATUS		# Restore KX
	COP0_SYNC
	sw	zero, PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(baderr64)
#endif
