/* $NetBSD: fpu.h,v 1.8 2021/07/22 01:39:18 thorpej Exp $ */

/*-
 * Copyright (c) 2001 Ross Harvey
 * All rights reserved.
 *
 * This software was written for NetBSD.
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

#ifndef _ALPHA_FPU_H_
#define _ALPHA_FPU_H_

/*
 * Most of these next definitions were moved from <ieeefp.h>. Apparently the
 * names happen to match those exported by Compaq and Linux from their fpu.h
 * files.
 */

/*
 * Bits in the Alpha Floating Point Control register.  This is the hardware
 * register, and should not be directly manipulated by application software.
 */
#define	FPCR_SUM	__BIT(63)	/* Summary (OR of all exception bits) */
#define	FPCR_INED	__BIT(62)	/* Inexact trap Disable */
#define	FPCR_UNFD	__BIT(61)	/* Underflow trap Disable */
#define	FPCR_UNDZ	__BIT(60)	/* Underflow to Zero */
#define	FPCR_DYN_RM	__BITS(58,59)	/* Dynamic Rounding Mode */
					/* 00 Chopped */
					/* 01 Minus Infinity */
					/* 10 Normal (round nearest) */
					/* 11 Plus Infinity */
#define	FPCR_IOV	__BIT(57)	/* Integer Overflow */
#define	FPCR_INE	__BIT(56)	/* Inexact Result */
#define	FPCR_UNF	__BIT(55)	/* Underflow */
#define	FPCR_OVF	__BIT(54)	/* Overflow */
#define	FPCR_DZE	__BIT(53)	/* Division By Zero */
#define	FPCR_INV	__BIT(52)	/* Invalid Operation */
#define	FPCR_OVFD	__BIT(51)	/* Overflow trap Disable */
#define	FPCR_DZED	__BIT(50)	/* Division By Zero trap Disable */
#define	FPCR_INVD	__BIT(49)	/* Invalid Operation trap Disable */
#define	FPCR_DNZ	__BIT(48)	/* Denormal Operands to Zero */
#define	FPCR_DNOD	__BIT(47)	/* Denormal Operation tap Disable */

#define	FPCR_MIRRORED (FPCR_INE | FPCR_UNF | FPCR_OVF | FPCR_DZE | FPCR_INV)
#define FPCR_MIR_START 52

/* NetBSD default - no traps enabled, round-to-nearest */
#define	FPCR_DEFAULT	(__SHIFTIN(FP_RN, FPCR_DYN_RM) |		\
			 FPCR_INED | FPCR_UNFD | FPCR_OVFD |		\
			 FPCR_DZED | FPCR_INVD | FPCR_DNOD)

/*
 * IEEE Floating Point Control (FP_C) Quadword.  This is a software
 * virtual register that abstracts the FPCR and software complation
 * performed by the kernel.
 *
 * The AARM specifies the bit positions of the software word used for
 * user mode interface to the control and status of the kernel completion
 * routines. Although it largely just redefines the FPCR, it shuffles
 * the bit order. The names of the bits are defined in the AARM, and
 * the definition prefix can easily be determined from public domain
 * programs written to either the Compaq or Linux interfaces, which
 * appear to be identical.
 *
 * Bits 63-48 are reserved for implementation software.
 * Bits 47-23 are reserved for future archiecture definition.
 * Bits 16-12 are reserved for implementation software.
 * Bits 11-7 are reserved for future architecture definition.
 * Bit 0 is reserved for implementation software.
 */

#define	IEEE_STATUS_DNO __BIT(22)	/* Denormal Operand */
#define	IEEE_STATUS_INE __BIT(21)	/* Inexact Result */
#define	IEEE_STATUS_UNF __BIT(20)	/* Underflow */
#define	IEEE_STATUS_OVF __BIT(19)	/* Overflow */
#define	IEEE_STATUS_DZE __BIT(18)	/* Division By Zero */
#define	IEEE_STATUS_INV __BIT(17)	/* Invalid Operation */

#define	IEEE_TRAP_ENABLE_DNO __BIT(6)	/* Denormal Operation trap */
#define	IEEE_TRAP_ENABLE_INE __BIT(5)	/* Inexact Result trap */
#define	IEEE_TRAP_ENABLE_UNF __BIT(4)	/* Underflow trap */
#define	IEEE_TRAP_ENABLE_OVF __BIT(3)	/* Overflow trap */
#define	IEEE_TRAP_ENABLE_DZE __BIT(2)	/* Division By Zero trap */
#define	IEEE_TRAP_ENABLE_INV __BIT(1)	/* Invalid Operation trap */

#define	IEEE_INHERIT __BIT(14)
#define	IEEE_MAP_UMZ __BIT(13)		/* Map underflowed outputs to zero */
#define	IEEE_MAP_DMZ __BIT(12)		/* Map denormal inputs to zero */

#define	FP_C_ALLBITS	__BITS(1,22)

#define	FP_C_MIRRORED	(IEEE_STATUS_INE | IEEE_STATUS_UNF | IEEE_STATUS_OVF \
			 | IEEE_STATUS_DZE | IEEE_STATUS_INV)
#define	FP_C_MIR_START 17

/* NetBSD default - no traps enabled (see FPCR default) */
#define	FP_C_DEFAULT	0

#ifdef _KERNEL

#define	FLD_MASK(len) ((1UL << (len)) - 1)
#define FLD_CLEAR(obj, origin, len)	\
		((obj) & ~(FLD_MASK(len) << (origin)))
#define	FLD_INSERT(obj, origin, len, value)	\
		(FLD_CLEAR(obj, origin, len) | (value) << origin)

#define	FP_C_TO_NETBSD_MASK(fp_c) 	((fp_c) >> 1 & 0x3f)
#define	FP_C_TO_NETBSD_FLAG(fp_c) 	((fp_c) >> 17 & 0x3f)
#define NETBSD_MASK_TO_FP_C(m)		(((m) & 0x3f) << 1)
#define NETBSD_FLAG_TO_FP_C(s)		(((s) & 0x3f) << 17)
#define	CLEAR_FP_C_MASK(fp_c)		((fp_c) & ~(0x3f << 1))
#define	CLEAR_FP_C_FLAG(fp_c)		((fp_c) & ~(0x3f << 17))
#define	SET_FP_C_MASK(fp_c, m) (CLEAR_FP_C_MASK(fp_c) | NETBSD_MASK_TO_FP_C(m))
#define	SET_FP_C_FLAG(fp_c, m) (CLEAR_FP_C_FLAG(fp_c) | NETBSD_FLAG_TO_FP_C(m))

#endif /* _KERNEL */

#endif /* _ALPHA_FPU_H_ */
