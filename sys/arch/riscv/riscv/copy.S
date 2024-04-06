/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <machine/asm.h>
#include "assym.h"

#define	UCAS_FRAME_SIZE	(FB_LEN + 4 * SZREG)
#define	UCAS_FRAME_A0	(UCAS_FRAME_SIZE - 4 * SZREG)
#define	UCAS_FRAME_A1	(UCAS_FRAME_SIZE - 3 * SZREG)
#define	UCAS_FRAME_S0	(UCAS_FRAME_SIZE - 2 * SZREG)
#define	UCAS_FRAME_RA	(UCAS_FRAME_SIZE - 1 * SZREG)

.macro enter_ucas
	addi	sp, sp, -UCAS_FRAME_SIZE
	REG_S	a0, UCAS_FRAME_A0(sp)
	REG_S	a1, UCAS_FRAME_A1(sp)
	REG_S	s0, UCAS_FRAME_S0(sp)
	REG_S	ra, UCAS_FRAME_RA(sp)
	addi	s0, sp, UCAS_FRAME_SIZE
.endm

.macro exit_ucas
	REG_L	s0, UCAS_FRAME_S0(sp)
	REG_L	ra, UCAS_FRAME_RA(sp)
	addi	sp, sp, UCAS_FRAME_SIZE
.endm

.macro enter_cpu_onfault
	// error = cpu_set_onfault(&fb, EFAULT);
	mv	a0, sp
	li	a1, EFAULT
	call	cpu_set_onfault
	// if (error) goto fail;
	bnez	a0, 9f
.endm

.macro exit_cpu_onfault
	// curlwp->l_md.md_onfault = NULL;
	REG_S	zero, L_MD_ONFAULT(tp)
.endm

.macro set_sum
	// csr_sstatus_set(SR_SUM);
	li	t1, SR_SUM
	csrs	sstatus, t1
.endm

.macro clear_sum
	// csr_sstatus_clear(SR_SUM);
	li	t1, SR_SUM
	csrc	sstatus, t1
.endm


/*
 * int _ucas_32(volatile uint32_t *ptr, uint32_t old,
 *	uint32_t new, uint32_t *ret)
 *
 *	Implies release/acquire barriers until someone tells me
 *	otherwise about _ucas_32/64.
 */
ENTRY(_ucas_32)
	li	t0, (VM_MAXUSER_ADDRESS - 4)
	bltu	t0, a0, 3f

	enter_ucas

	enter_cpu_onfault

	REG_L	t0, UCAS_FRAME_A0(sp)
	REG_L	a1, UCAS_FRAME_A1(sp)

	set_sum

1:	lr.w	a0, 0(t0)		/* load old value		 */
	bne	a0, a1, 2f		/*     return if different	 */
	sc.w	t1, a2, 0(t0)		/* store new value		 */
	bnez	t1, 1b			/*     succeed? nope, try again. */
2:
	clear_sum

	sw	a0, 0(a3)

	exit_cpu_onfault
	li	a0, 0			// success
9:
	exit_ucas
	ret
3:
	li	a0, EFAULT
	ret
END(_ucas_32)


#ifdef _LP64
/*
 * int _ucas_64(volatile uint64_t *ptr, uint64_t old,
 *	uint64_t new, uint64_t *ret)
 *
 *	Implies release/acquire barriers until someone tells me
 *	otherwise about _ucas_32/64.
 */
ENTRY(_ucas_64)
	li	t0, (VM_MAXUSER_ADDRESS - 8)
	bltu	t0, a0, 3f

	enter_ucas

	enter_cpu_onfault

	REG_L	t0, (FB_LEN + 0 * SZREG)(sp)
	REG_L	a1, (FB_LEN + 1 * SZREG)(sp)

	set_sum

1:	lr.d	a0, 0(t0)		/* load old value		 */
	bne	a1, a0, 2f		/*     return if different	 */
	sc.d	t1, a2, 0(t0)		/* store new value		 */
	bnez	t1, 1b			/*     succeed? nope, try again. */
2:
	clear_sum

	sd	a0, 0(a3)

	exit_cpu_onfault
	li	a0, 0			// success
9:
	exit_ucas
	ret
3:
	li	a0, EFAULT
	ret
END(_ucas_64)
#endif
