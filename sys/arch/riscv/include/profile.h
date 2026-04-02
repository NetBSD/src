/* $NetBSD: profile.h,v 1.1.60.1 2026/04/02 15:59:59 martin Exp $ */

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

#ifndef _RISCV_PROFILE_H_
#define _RISCV_PROFILE_H_

#include <machine/sysreg.h>

#define	_MCOUNT_DECL void mcount

#define MCOUNT_ASM_NAME "_mcount"
#define	PLTSYM

#ifdef _LP64
#define	MCOUNT	MCOUNT_ARCH("8", "sd", "ld")
#else
#define	MCOUNT	MCOUNT_ARCH("4", "sw", "lw")
#endif

#ifdef __PIC__
#define	_PLT	"@plt"
#else
#define	_PLT	/* nothing */	
#endif


#define	MCOUNT_ARCH(_SZREG, _REG_S, _REG_L)	__asm(			\
"	.text\n"							\
"	.align	2\n"							\
"	.type	" MCOUNT_ASM_NAME ",@function\n"			\
"	.global	" MCOUNT_ASM_NAME "\n"					\
MCOUNT_ASM_NAME ":\n"							\
	/*								\
	 * Preserve registers that could be trashed during mcount, i.e.	\
	 * caller saved registers.					\
	 */								\
"	addi	sp, sp, -(16 * " _SZREG ")\n"				\
"	" _REG_S "	ra, ( 0 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t0, ( 1 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t1, ( 2 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t2, ( 3 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a0, ( 4 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a1, ( 5 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a2, ( 6 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a3, ( 7 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a4, ( 8 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a5, ( 9 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a6, (10 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	a7, (11 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t3, (12 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t4, (13 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t5, (14 * " _SZREG ")(sp)\n" 			\
"	" _REG_S "	t6, (15 * " _SZREG ")(sp)\n" 			\
"	mv	a1, ra\n"						\
"	call	mcount " _PLT "\n"					\
	 /* restore caller saved registers */				\
"	" _REG_L "	ra, ( 0 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t0, ( 1 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t1, ( 2 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t2, ( 3 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a0, ( 4 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a1, ( 5 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a2, ( 6 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a3, ( 7 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a4, ( 8 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a5, ( 9 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a6, (10 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	a7, (11 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t3, (12 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t4, (13 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t5, (14 * " _SZREG ")(sp)\n"			\
"	" _REG_L "	t6, (15 * " _SZREG ")(sp)\n"			\
"	addi	sp, sp, (16 * " _SZREG ")\n"				\
"	jr	ra\n");


#ifdef _KERNEL

#define MCOUNT_ENTER						\
   s = (csr_sstatus_read() & SR_SIE) != 0;			\
   csr_sstatus_clear(SR_SIE);	// DISABLE_INTERRUPTS

#define	MCOUNT_EXIT						\
   if (s)							\
   	csr_sstatus_set(SR_SIE); // ENABLE_INTERRUPTS

#endif

#endif /* _RISCV_PROFILE_H_ */

