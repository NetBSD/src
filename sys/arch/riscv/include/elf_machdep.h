/* $NetBSD: elf_machdep.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */

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

#ifndef _RISCV_ELF_MACHDEP_H_
#define _RISCV_ELF_MACHDEP_H_

#define	ELF32_MACHDEP_ID		EM_RISCV

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define ELF64_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF32_MACHDEP_ID_CASES		\
		case EM_RISCV:		\
			break;

#define	ELF64_MACHDEP_ID_CASES		\
		case EM_RISCV:		\
			break;

#ifdef _LP64
#define ARCH_ELFSIZE		64	/* MD native binary size */
#else
#define ARCH_ELFSIZE		32	/* MD native binary size */
#endif

/* Processor specific flags for the ELF header e_flags field.  */

/* Processor specific relocation types */

#define R_RISCV_NONE		0
#define R_RISCV_32		2	// A
#define R_RISCV_REL32		3	// A & 0xffff
#define R_RISCV_JAL		4	// A & 0xff
#define R_RISCV_HI20		5	// A & 0xffff
#define R_RISCV_LO12_I		6	// (A >> 16) & 0xffff
#define R_RISCV_LO12_S		7	// (S + A - P) >> 2
#define R_RISCV_PCREL_LO12_I	8
#define R_RISCV_PCREL_LO12_S	9
#define R_RISCV_BRANCH		10	// (A - P) & 0xffff
#define R_RISCV_CALL		11	// (A - P) & 0xff
#define R_RISCV_PCREL_HI20	12
#define R_RISCV_CALL_PLT	13
#define R_RISCV_64		18
#define R_RISCV_GOT_HI20	22
#define R_RISCV_GOT_LO12	23
#define R_RISCV_COPY		24
#define R_RISCV_JMP_SLOT	25
/* TLS relocations. */
#define R_RISCV_TLS_IE_HI20	29
#define R_RISCV_TLS_IE_LO12	30
#define R_RISCV_TLS_IE_ADD	31
#define R_RISCV_TLS_IE_LO12_I	32
#define R_RISCV_TLS_IE_LO12_S	33
#define R_RISCV_TPREL_HI20	34
#define R_RISCV_TLREL_LO12_I	35
#define R_RISCV_TLREL_LO12_S	36
#define R_RISCV_TLREL_ADD	37

#define R_RISCV_TLS_DTPMOD32	38
#define R_RISCV_TLS_DTPREL32	39
#define R_RISCV_TLS_DTPMOD64	40
#define R_RISCV_TLS_DTPREL64	41
#define R_RISCV_TLS_DTPREL_HI16	44
#define R_RISCV_TLS_DTPREL_LO16	45	
#define R_RISCV_TLS_PCREL_LO12	50
#define R_RISCV_TLS_GOT_HI20	51
#define R_RISCV_TLS_GOT_LO20	52
#define R_RISCV_TLS_GD_HI20	53
#define R_RISCV_TLS_GD_LO20	54

#define R_RISCV_GLOB_DAT	57
#define R_RISCV_ADD32		58
#define R_RISCV_SUB32		59
#define R_RISCV_ADD64		60
#define R_RISCV_SUB64		61

#define R_TYPE(name)		R_RISCV_ ## name
#define R_TLS_TYPE(name)	R_RISCV_ ## name ## 64

#define DT_RISCV_LOCAL_GOTNO	(DT_LOPROC + 0)
#define DT_RISCV_SYMTABNO	(DT_LOPROC + 1)
#define DT_RISCV_GOTSYM		(DT_LOPROC + 2)
#define DT_RISCV_PLTGOT		(DT_LOPROC + 3)

#ifdef _KERNEL
#ifdef ELFSIZE
#define ELF_MD_PROBE_FUNC       ELFNAME2(cpu_netbsd,probe)
#endif     
 
struct exec_package;
 
int cpu_netbsd_elf32_probe(struct lwp *, struct exec_package *, void *, char *,
        vaddr_t *); 
 
int cpu_netbsd_elf64_probe(struct lwp *, struct exec_package *, void *, char *,
        vaddr_t *);

#endif /* _KERNEL */

#endif /* _RISCV_ELF_MACHDEP_H_ */
