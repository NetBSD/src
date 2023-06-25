/* $NetBSD: elf_machdep.h,v 1.2 2017/11/06 03:47:47 christos Exp $ */

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

#ifndef _OR1K_ELF_MACHDEP_H_
#define _OR1K_ELF_MACHDEP_H_

#define	ELF32_MACHDEP_ID	EM_OR1K

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define ELF64_MACHDEP_ENDIANNESS	xxx

#define ELF32_MACHDEP_ID_CASES                                          \
		case EM_OR1K:					\
			break;

#define	ELF64_MACHDEP_ID_CASES

#define	KERN_ELFSIZE		32
#define ARCH_ELFSIZE		32	/* MD native binary size */

/* Processor specific flags for the ELF header e_flags field.  */

#define EF_OR1K_NODELAY		0x00000001

/* Processor specific relocation types */

#define R_OR1K_NONE		0
#define R_OR1K_32		1	// A
#define R_OR1K_16		2	// A & 0xffff
#define R_OR1K_8		3	// A & 0xff
#define R_OR1K_LO_16_IN_INSN	4	// A & 0xffff
#define R_OR1K_HI_16_IN_INSN	5	// (A >> 16) & 0xffff
#define R_OR1K_INSN_REL_26	6	// (S + A - P) >> 2
#define R_OR1K_GNU_VTENTRY	7
#define R_OR1K_GNU_VTINHERIT	8
#define R_OR1K_32_PCREL		9	// A - P
#define R_OR1K_16_PCREL		10	// (A - P) & 0xffff
#define R_OR1K_8_PCREL		11	// (A - P) & 0xff
#define R_OR1K_GOTPC_HI16	12
#define R_OR1K_GOTPC_LO16	13
#define R_OR1K_GOT15		14
#define R_OR1K_PLT26		15
#define R_OR1K_GOTOFF_HI16	16
#define R_OR1K_GOTOFF_LO16	17
#define R_OR1K_COPY		18
#define R_OR1K_GLOB_DAT		19
#define R_OR1K_JMP_SLOT		20
#define R_OR1K_RELATIVE		21
#define R_OR1K_TLS_GD_HI16	22
#define R_OR1K_TLS_GD_LO16	23
#define R_OR1K_TLS_LDM_HI16	24
#define R_OR1K_TLS_LDM_LO16	25
#define R_OR1K_TLS_LDO_HI16	26
#define R_OR1K_TLS_LDO_LO16	27
#define R_OR1K_TLS_IE_HI16	28
#define R_OR1K_TLS_IE_LO16	29
#define R_OR1K_TLS_LE_HI16	30
#define R_OR1K_TLS_LE_LO16	31	
#define R_OR1K_TLS_TPOFF	32
#define R_OR1K_TLS_DTPOFF	33
#define R_OR1K_TLS_DTPMOD	34

#define R_TYPE(name)		R_OR1K_ ## name
#define R_TLS_TYPE(name)	R_OR1K_ ## name ## 64

#endif /* _OR1K_ELF_MACHDEP_H_ */
