/*	$NetBSD: _setjmp.S,v 1.10 1999/01/15 03:53:33 castor Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#include <machine/cdefs.h>
#include <mips/regnum.h>
#include <mips/asm.h>
#include <machine/setjmp.h>

#if defined(LIBC_SCCS) && !defined(lint)
	ASMSTR("from: @(#)_setjmp.s	8.1 (Berkeley) 6/4/93")
	ASMSTR("$NetBSD: _setjmp.S,v 1.10 1999/01/15 03:53:33 castor Exp $")
#endif /* LIBC_SCCS and not lint */

#ifdef ABICALLS
	.abicalls
#endif

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	_setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */

	.set	noreorder

LEAF(_setjmp)
#ifdef ABICALLS 
	#.set noreorder
	.cpload t9
	#.set reorder
#endif

	REG_PROLOGUE
	REG_LI	v0, 0xACEDBADE			# sigcontext magic number
	REG_S	ra, (2 * 4)(a0)			# sc_pc = return address
	REG_S	v0, (_OFFSETOF_SC_REGS)(a0)	#   saved in sc_regs[0]
	REG_S	s0, (S0 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s1, (S1 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s2, (S2 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s3, (S3 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s4, (S4 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s5, (S5 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s6, (S6 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s7, (S7 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	sp, (SP * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_S	s8, (S8 * SZREG + _OFFSETOF_SC_REGS)(a0)
	cfc1	v0, $31				# too bad cant check if FP used
	swc1	$f20, (20 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f21, (21 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f22, (22 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f23, (23 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f24, (24 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f25, (25 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f26, (26 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f27, (27 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f28, (28 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f29, (29 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f30, (30 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	swc1	$f31, (31 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	sw	v0, (32 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	REG_EPILOGUE
	j	ra
	move	v0, zero
END(_setjmp)

LEAF(_longjmp)
#ifdef ABICALLS 
	.set    noreorder
	.cpload t9
	.set    reorder
	subu	sp, sp, 32
	.cprestore 16
	.set    noreorder
#endif  
	REG_PROLOGUE
	REG_L	v0, (_OFFSETOF_SC_REGS)(a0)	# get magic number
	REG_L	ra, (2 * 4)(a0)
	REG_LI	t0, 0xACEDBADE
	bne	v0, t0, botch		# jump if error
	addu	sp, sp, 32			# does not matter, sanity
	REG_L	s0, (S0 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s1, (S1 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s2, (S2 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s3, (S3 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s4, (S4 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s5, (S5 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s6, (S6 * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s7, (S7 * SZREG + _OFFSETOF_SC_REGS)(a0)
	lw	v0, (32 * 4 + _OFFSETOF_SC_FPREGS)(a0)	# get fpu status
	REG_L	sp, (SP * SZREG + _OFFSETOF_SC_REGS)(a0)
	REG_L	s8, (S8 * SZREG + _OFFSETOF_SC_REGS)(a0)
	ctc1	v0, $31
	lwc1	$f20, (20 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f21, (21 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f22, (22 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f23, (23 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f24, (24 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f25, (25 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f26, (26 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f27, (27 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f28, (28 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f29, (29 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f30, (30 * 4 + _OFFSETOF_SC_FPREGS)(a0)
	lwc1	$f31, (31 * 4 + _OFFSETOF_SC_FPREGS)(a0)

	j	ra
	move	v0, a1
	REG_EPILOGUE
botch:
	jal	_C_LABEL(longjmperror)
	nop
	jal	_C_LABEL(abort)
	nop
END(_longjmp)
