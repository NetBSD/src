/*	$NetBSD: elf_machdep.h,v 1.1 2001/06/19 00:20:10 fvdl Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_386:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF64_MACHDEP_ID_CASES						\
		case EM_X8664:						\
			break;

#define ARCH_ELFSIZE		64	/* MD native binary size */

/* x86-64 relocations */

#define R_X86_64_NONE		0
#define R_X86_64_64		1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4
#define R_X86_64_COPY		5
#define R_X86_64_GLOB_DAT	6
#define R_X86_64_JUMP_SLOT	7
#define R_X86_64_RELATIVE	8
#define R_X86_64_GOTPCREL	9
#define R_X86_64_32		10
#define R_X86_64_32S		11
#define R_X86_64_16		12
#define R_X86_64_PC16		13
#define R_X86_64_8		14
#define R_X86_64_PC8		15

#define	R_TYPE(name)	__CONCAT(R_X86_64_,name)
