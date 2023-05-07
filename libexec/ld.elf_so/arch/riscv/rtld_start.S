/*	$NetBSD: rtld_start.S,v 1.6 2023/05/07 12:41:48 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

	.globl _C_LABEL(_rtld_relocate_nonplt_self)
	.globl _C_LABEL(_rtld)

#define PTRSZ	__SIZEOF_POINTER__

/*
 * void
 * ___start(
 *     void (*cleanup)(void),
 *     struct ps_strings *ps_strings
 * )
 */

ENTRY(_rtld_start)
	mv	s0, sp			# save stack pointer
	addi	sp, sp, -4 * PTRSZ	# adjust stack pointer
					# -> 2 * PTR_SIZE(sp) for atexit
					# -> 3 * PTR_SIZE(sp) for obj_main
	mv	s1, a1			# save ps_strings pointer

.L0:	auipc	gp, %pcrel_hi(_GLOBAL_OFFSET_TABLE_)
	PTR_L	t0, %pcrel_lo(.L0)(gp) # &_DYNAMIC
.L1:	auipc	a0, %pcrel_hi(_DYNAMIC)
	addi	a0, a0, %pcrel_lo(.L1)
	sub	s2, a0, t0		# save for _rtld
	mv	a1, s2
	call	_C_LABEL(_rtld_relocate_nonplt_self)

	mv	a1, s2			# relocbase
	addi	a0, sp, 2 * PTRSZ	# sp
	call	_C_LABEL(_rtld)		# a0 = _rtld(sp, relocbase)
	mv	t0, a0

	PTR_L	a0, 2 * PTRSZ(sp)	# cleanup function
//	PTR_L	a1, 3 * PTRSZ(sp)	# obj_main pointer (not used)
	mv	a1, s1			# restore ps_strings
	mv	sp, s0			# restore stack pointer
	mv	s0, zero		# break stack chain
	jr	t0			# _start(cleanup, ps_strings);
END(_rtld_start)

/*
 * Needs to be 12 so that stack alignment is preserved on both RV32 and RV64.
 */

#define	XCALLFRAME_SIZ		(12 * SZREG)
#define	XCALLFRAME_RA		( 8 * SZREG)
#define	XCALLFRAME_A7		( 7 * SZREG)
#define	XCALLFRAME_A6		( 6 * SZREG)
#define	XCALLFRAME_A5		( 5 * SZREG)
#define	XCALLFRAME_A4		( 4 * SZREG)
#define	XCALLFRAME_A3		( 3 * SZREG)
#define	XCALLFRAME_A2		( 2 * SZREG)
#define	XCALLFRAME_A1		( 1 * SZREG)
#define	XCALLFRAME_A0		( 0 * SZREG)

/*
 * t0 = obj pointer
 * t1 = reloc offset
 */
ENTRY_NP(_rtld_bind_start)
	addi	sp, sp, -XCALLFRAME_SIZ	// save arguments on stack
	REG_S	a0,  XCALLFRAME_A0(sp)
	REG_S	a1,  XCALLFRAME_A1(sp)
	REG_S	a2,  XCALLFRAME_A2(sp)
	REG_S	a3,  XCALLFRAME_A3(sp)
	REG_S	a4,  XCALLFRAME_A4(sp)
	REG_S	a5,  XCALLFRAME_A5(sp)
	REG_S	a6,  XCALLFRAME_A6(sp)
	REG_S	a7,  XCALLFRAME_A7(sp)
	REG_S	ra,  XCALLFRAME_RA(sp)

	mv	a0, t0			/* object from got.plt[1] */
	mv	a1, t1			/* reloc offset */

	call	_C_LABEL(_rtld_bind)
	mv	t0, a0			/* save function pointer */

	REG_L	a0, XCALLFRAME_A0(sp)
	REG_L	a1, XCALLFRAME_A1(sp)
	REG_L	a2, XCALLFRAME_A2(sp)
	REG_L	a3, XCALLFRAME_A3(sp)
	REG_L	a4, XCALLFRAME_A4(sp)
	REG_L	a5, XCALLFRAME_A5(sp)
	REG_L	a6, XCALLFRAME_A6(sp)
	REG_L	a7, XCALLFRAME_A7(sp)
	REG_L	ra, XCALLFRAME_RA(sp)
	addi	sp, sp, XCALLFRAME_SIZ
	jr	t0
END(_rtld_bind_start)
