/*	$NetBSD: locore_mips3.S,v 1.102.30.2 2015/09/22 12:05:47 skrll Exp $	*/

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
#include "opt_multiprocessor.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>
#include <sys/endian.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

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
#if (__mips < 3) || __mips_o32
	.set	mips3
#endif

#ifdef _LP64
#define	_MFC0	dmfc0
#define	_MTC0	dmtc0
#else
#define	_MFC0	mfc0
#define	_MTC0	mtc0
#endif

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
XLEAF(loongson2_wbflush)
XLEAF(mips32_wbflush)
XLEAF(mips32r2_wbflush)
XLEAF(mips64_wbflush)
XLEAF(mips64r2_wbflush)
	nop
	sync
	jr	ra
	 nop
END(mips3_wbflush)


/*
 * mips_wait_idle:
 *
 *	When no processes are on the runq, cpu_idle branches to
 *	mips_wait_idle to save power.
 */
LEAF(mips_wait_idle)
	mfc0	v0, MIPS_COP_0_STATUS
	andi	v1, v0, MIPS_SR_INT_IE
#if __mips >= 32
	teqi	v1, 0
#else
	beqz	v1, 1f
	 nop
#endif
	andi	v1, v0, MIPS_INT_MASK
#if __mips >= 32
	teqi	v1, 0
#else
	bnez	v1, 2f
	 nop
1:
	move	a1, v0
	PANIC("mips_wait_idle: interrupts disabled status=%#x")
#endif

2:	wait
	nop
	nop
	nop
	mfc0	v0, MIPS_COP_0_STATUS
	andi	v1, v0, MIPS_SR_INT_IE
#if __mips >= 32
	teqi	v1, 0
#else
	beqz	v1, 1b
	 nop
#endif
	andi	v1, v0, MIPS_INT_MASK
#if __mips >= 32
	teqi	v1, 0
#else
	beqz	v1, 1b
	 nop
#endif
	jr	ra
	 nop
END(mips_wait_idle)

/*
 * uint32_t mips3_cp0_compare_read(void)
 *
 *	Return the current value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_read)
	mfc0	v0, MIPS_COP_0_COMPARE
	jr	ra
	 nop
END(mips3_cp0_compare_read)

/*
 * void mips3_cp0_compare_write(uint32_t)
 *
 *	Set the value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_write)
	mtc0	a0, MIPS_COP_0_COMPARE
	JR_HB_RA
END(mips3_cp0_compare_write)

/*
 * uint32_t mips3_cp0_config_read(void)
 *
 *	Return the current value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_read)
	mfc0	v0, MIPS_COP_0_CONFIG
	jr	ra
	 nop
END(mips3_cp0_config_read)

/*
 * void mips3_cp0_config_write(uint32_t)
 *
 *	Set the value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_write)
	mtc0	a0, MIPS_COP_0_CONFIG
	JR_HB_RA
END(mips3_cp0_config_write)

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	.set push
#ifdef __mips_o32
	.set mips32
#else
	.set mips64
#endif
/*
 * uint32_t mipsNN_cp0_config1_read(void)
 *
 *	Return the current value of the CP0 Config (Select 1) register.
 */
LEAF(mipsNN_cp0_config1_read)
	mfc0	v0, MIPS_COP_0_CONFIG, 1
	jr	ra
	 nop
END(mipsNN_cp0_config1_read)

/*
 * uint32_t mipsNN_cp0_config1_write(uint32_t)
 *
 *	Set the current value of the CP0 Config (Select 1) register.
 */
LEAF(mipsNN_cp0_config1_write)
	mtc0	v0, MIPS_COP_0_CONFIG, 1
	JR_HB_RA
END(mipsNN_cp0_config1_write)

/*
 * uint32_t mipsNN_cp0_config2_read(void)
 *
 *	Return the current value of the CP0 Config (Select 2) register.
 */
LEAF(mipsNN_cp0_config2_read)
	mfc0	v0, MIPS_COP_0_CONFIG, 2
	jr	ra
	 nop
END(mipsNN_cp0_config2_read)

/*
 * uint32_t mipsNN_cp0_config3_read(void)
 *
 *	Return the current value of the CP0 Config (Select 3) register.
 */
LEAF(mipsNN_cp0_config3_read)
	mfc0	v0, MIPS_COP_0_CONFIG, 3
	jr	ra
	 nop
END(mipsNN_cp0_config3_read)

/*
 * uintptr_t mipsNN_cp0_watchlo_read(u_int sel)
 *
 *	Return the current value of the selected CP0 Watchlo register.
 */
LEAF(mipsNN_cp0_watchlo_read)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 0
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 1
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 2
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 3
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 4
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 5
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 6
	jr	ra
	 _MFC0	v0, MIPS_COP_0_WATCH_LO, 7
END(mipsNN_cp0_watchlo_read)

/*
 * void mipsNN_cp0_watchlo_write(u_int sel, uintptr_t val)
 *
 *	Set the current value of the selected CP0 WatchLo register.
 */
LEAF(mipsNN_cp0_watchlo_write)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 0
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 1
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 2
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 3
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 4
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 5
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 6
	jr	ra
	 _MTC0	a1, MIPS_COP_0_WATCH_LO, 7
END(mipsNN_cp0_watchlo_write)

/*
 * uint32_t mipsNN_cp0_watchhi_read(u_int sel)
 *
 *	Return the current value of the selected CP0 WatchHi register.
 */
LEAF(mipsNN_cp0_watchhi_read)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 0
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 1
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 2
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 3
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 4
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 5
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 6
	jr	ra
	 mfc0	v0, MIPS_COP_0_WATCH_HI, 7
END(mipsNN_cp0_watchhi_read)

/*
 * void mipsNN_cp0_watchhi_write(u_int sel, uint32_t val)
 *
 *	Set the current value of the selected CP0 WatchHi register.
 */
LEAF(mipsNN_cp0_watchhi_write)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 0
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 1
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 2
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 3
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 4
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 5
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 6
	jr	ra
	 mtc0	a1, MIPS_COP_0_WATCH_HI, 7
END(mipsNN_cp0_watchhi_write)

/*
 * void mipsNN_cp0_ebase_read(void *);
 *	Get the value of the CP0 EBASE (PRID, select 1) register.
 */
LEAF(mipsNN_cp0_ebase_read)
	jr	ra
	 mfc0	v0, MIPS_COP_0_EBASE
END(mipsNN_cp0_ebase_read)

/*
 * void mipsNN_cp0_ebase_write(void *);
 *	Set the value of the CP0 EBASE (PRID, select 1) register.
 */
LEAF(mipsNN_cp0_ebase_write)
	and	v0, v0, 0x1ff
	xor	v0, v0, a0
	jr	ra
	 mtc0	v0, MIPS_COP_0_EBASE
END(mipsNN_cp0_ebase_write)

#if (MIPS32R2 + MIPS64R2) > 0
/*
 * void mipsNN_cp0_hwrena_write(void *);
 *	Set the value of the CP0 HWRENA register.
 */
LEAF(mipsNN_cp0_hwrena_write)
	jr	ra
	 mtc0	a0, MIPS_COP_0_HWRENA
END(mipsNN_cp0_hwrena_write)

/*
 * void mipsNN_cp0_userlocal_write(void *);
 *	Set the value of the CP0 USERLOCAL (TLB_CONTEXT, select 2) register.
 */
LEAF(mipsNN_cp0_userlocal_write)
	jr	ra
	 _MTC0	a0, MIPS_COP_0_USERLOCAL
END(mipsNN_cp0_userlocal_write)
#endif /* (MIPS32R2 + MIPS64R2) > 0 */
	.set	pop
#endif /* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */

/*
 * uint32_t mips3_cp0_count_read(void)
 *
 *	Return the current value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_read)
	mfc0	v0, MIPS_COP_0_COUNT
	jr	ra
	 nop
END(mips3_cp0_count_read)
WEAK_ALIAS(cpu_counter32, mips3_cp0_count_read)

/*
 * void mips3_cp0_count_write(uint32_t)
 *
 *	Set the value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_write)
	mtc0	a0, MIPS_COP_0_COUNT
	JR_HB_RA
END(mips3_cp0_count_write)

/*
 * uint32_t mips3_cp0_wired_read(void)
 *
 *	Return the current value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_read)
	mfc0	v0, MIPS_COP_0_TLB_WIRED
	jr	ra
	 nop
END(mips3_cp0_wired_read)

/*
 * void mips3_cp0_wired_write(uint32_t)
 *
 *	Set the value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_write)
	mtc0	a0, MIPS_COP_0_TLB_WIRED
	JR_HB_RA
END(mips3_cp0_wired_write)

/*
 * void mips3_cp0_pg_mask_write(uint32_t)
 *
 *	Set the value of the CP0 PG_MASK register.
 */
LEAF(mips3_cp0_pg_mask_write)
	mtc0	a0, MIPS_COP_0_TLB_PG_MASK
	JR_HB_RA
END(mips3_cp0_pg_mask_write)

#if __mips != 32
LEAF(mips3_ld)
#if defined(__mips_o32)
	mfc0	t0, MIPS_COP_0_STATUS		# turn off interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC

	ld	v0, 0(a0)
#if defined(_MIPS_BSD_API) && \
    (_MIPS_BSD_API == _MIPS_BSD_API_N32 || _MIPS_BSD_API == _MIPS_BSD_API_LP64)
	# nothing to do
#else
#if _BYTE_ORDER == _BIG_ENDIAN
	dsll	v1, v0, 32
	dsra	v1, v1, 32			# low word in v1
	dsra	v0, v0, 32			# high word in v0
#else
	dsra	v1, v0, 32			# high word in v1
	dsll	v0, v0, 32
	dsra	v0, v0, 32			# low word in v0
#endif
#endif

	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	JR_HB_RA
#else /* !__mips_o32 */
	ld	v0, 0(a0)
	jr	ra
	 nop
#endif /* !__mips_o32 */
END(mips3_ld)

LEAF(mips3_sd)
#if defined(__mips_o32)
	mfc0	t0, MIPS_COP_0_STATUS		# turn off interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC

#if defined(_MIPS_BSD_API) && \
    (_MIPS_BSD_API == _MIPS_BSD_API_N32 || _MIPS_BSD_API == _MIPS_BSD_API_LP64)
	# nothing to do
#else
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
#endif
	sd	a1, 0(a0)

	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	JR_HB_RA
#else /* !__mips_o32 */
	sd	a1, 0(a0)
	jr	ra
	 nop
#endif /* !__mips_o32 */

END(mips3_sd)

/*
 * int badaddr64(uint64_t addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access in bytes (can be 1, 2, 4, or 8).
 */
LEAF(badaddr64)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(baderr64)

	/* Enable KX */
	mfc0	t0, MIPS_COP_0_STATUS
	or	t1, t0, MIPS3_SR_KX
#ifdef __mips_o32
	and	t2, t0, MIPS_SR_INT_IE	# disable interrupts
	xor	t1, t2
#endif
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC

#ifdef __mips_o32
#ifdef __MIPSEB__
	dsll	a0, a0, 32	# MSW
	dsll	a1, a1, 32	# LSW
	dsrl	a1, a1, 32
#else
	dsll	a1, a1, 32	# MSW
	dsll	a0, a0, 32	# LSW
	dsrl	a0, a0, 32
#endif
	or	a0, a1		# combine
	move	a1, a2		# move up length argument
#endif /* __mips_o32 */

	bne	a1, 1, 2f
	 PTR_S	v0, PCB_ONFAULT(v1)
	b	9f
	 lbu	v0, (a0)
2:
	bne	a1, 2, 4f
	 nop
	b	9f
	 lhu	v0, (a0)
4:
	bne	a1, 4, 8f
	 nop
	b	9f
	 INT_L	v0, (a0)
8:
	REG_L	v0, (a0)
9:
	sync
	mtc0	t0, MIPS_COP_0_STATUS           # Restore KX
	COP0_SYNC
	PTR_S	zero, PCB_ONFAULT(v1)
	jr	ra
	 move	v0, zero			# made it w/o errors
END(badaddr64)

LEAF(baderr64)
	mtc0	t0, MIPS_COP_0_STATUS		# Restore KX
	COP0_SYNC
	PTR_S	zero, PCB_ONFAULT(v1)
	jr	ra
	 li	v0, -1
END(baderr64)

/*
 * uint64_t mips3_cp0_tlb_entry_hi_probe(void);
 *
 * Write 1s to the VPN and ASID fields of Entry_Hi0 to see how many VA bits
 * and ASID bits are implemented.  Assumes that interrupts are disabled.
 */
LEAF(mips3_cp0_tlb_entry_hi_probe)
	dmfc0	t0, MIPS_COP_0_TLB_HI
	li	v0, -1		/* all 1s */
#if defined(__mips_isa_rev) && __mips_isa_rev >= 2
	dinsu	v0, zero, 62, 2
#else
	dsll	v0, v0, 2	/* except the top 2 */
	dsrl	v0, v0, 2
#endif
	dmtc0	v0, MIPS_COP_0_TLB_HI
	COP0_SYNC
	dmfc0	v0, MIPS_COP_0_TLB_HI
	dmtc0	t0, MIPS_COP_0_TLB_HI
	COP0_SYNC
	nop
#ifdef __mips_o32
	nop
#if BYTE_ORDER == BIG_ENDIAN
	srl	v1, v0, 0
	dsra	v0, v0, 32
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	dsra	v1, v0, 32
	srl	v0, v0, 0
#endif
#endif /* __mips_o32 */
	jr	ra
	 nop
END(mips3_cp0_tlb_entry_hi_probe)

/*
 * uint64_t mips3_cp0_tlb_entry_lo_probe(void);
 *
 * Write 1s to the PFN field of Entry_Lo0 to see how many
 * PA bits are implemented.  Assumes that interrupts are disabled.
 */
LEAF(mips3_cp0_tlb_entry_lo_probe)
	dmfc0	t0, MIPS_COP_0_TLB_LO0
	li	v0, -64		/* all 1s except low 6 bits */
	dmtc0	v0, MIPS_COP_0_TLB_LO0
	COP0_SYNC
	dmfc0	v0, MIPS_COP_0_TLB_LO0
	dmtc0	t0, MIPS_COP_0_TLB_LO0
	COP0_SYNC
#ifdef __mips_o32
#if BYTE_ORDER == BIG_ENDIAN
	srl	v1, v0, 0
	dsra	v0, v0, 32
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	dsra	v1, v0, 32
	srl	v0, v0, 0
#endif
#endif /* __mips_o32 */
	jr	ra
	 nop
END(mips3_cp0_tlb_entry_lo_probe)
#endif /* __mips != 32 */

/*
 * uint32_t mips3_cp0_tlb_page_mask_probe(void);
 *
 * Write 1s to the RPN field of Entry_Lo0 to see how many PA bits are implemented.
 * Assumes that interrupts are disabled.
 */
LEAF(mips3_cp0_tlb_page_mask_probe)
	mfc0	t0, MIPS_COP_0_TLB_PG_MASK
	lui	v0, 0xffff
	srl	v0, v0, 3
	mtc0	v0, MIPS_COP_0_TLB_PG_MASK
	COP0_SYNC
	mfc0	v0, MIPS_COP_0_TLB_PG_MASK
	mtc0	t0, MIPS_COP_0_TLB_PG_MASK
	JR_HB_RA
END(mips3_cp0_tlb_page_mask_probe)

#ifdef MULTIPROCESSOR
/*
 * The presence of this routine here will causes MIPS1 MULTIPROCESSOR kernels
 * to fail, as they should since we won't support such abominations.
 *
 * MD code calls/jumps to this with the pointer to this CPU's cpu_info in a1,
 * sp set to ci->ci_data.cpu_idlelwp->l_md.md_utf.  gp will be overridden so
 * 0 can be supplied if needed.  (This happens to match what CFE wants)
 */
NESTED_NOPROFILE(cpu_trampoline, 0, ra)
	/*
	 * We act as the idle lwp so make it CURLWP.  When know
	 * that the cpu_info is a KSEG0 address.
	 */
	move	a0, a1
	// Loop until idlelwp is filled in.
1:	PTR_L	MIPS_CURLWP, CPU_INFO_IDLELWP(a0)
	nop
	beqz	MIPS_CURLWP, 1b
	 nop
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(a0)

#ifdef _LP64
	li	v0, MIPS_SR_KX | MIPS_SR_UX	# allow 64bit addressing
#else
	li	v0, 0
#endif
	mtc0	v0, MIPS_COP_0_STATUS		# reset to known state
	COP0_SYNC

	PTR_L	sp, L_MD_UTF(MIPS_CURLWP)	# fetch KSP

	/*
	 * Indicate that no one has called us.
	 */
	move	ra, zero
	REG_S	ra, CALLFRAME_RA(sp)

#ifdef __GP_SUPPORT__
	/*
	 * New execution constant needs GP to be loaded.
	 */
	PTR_LA	gp, _C_LABEL(_gp)
#endif

#if 0
	LONG_L	t0, CPU_INFO_FLAGS(a0)
	or	t0, t0, CPUF_PRESENT
	LONG_S	t0, CPU_INFO_FLAGS(a0)
	sync
#endif

	/*
	 * and off we go.
	 */
	j	_C_LABEL(cpu_hatch)		# does everything
	 nop
END(cpu_trampoline)
#endif /* MULTIPROCESSOR */
