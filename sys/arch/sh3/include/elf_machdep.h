/*	$NetBSD: elf_machdep.h,v 1.1 1999/09/13 10:31:17 itojun Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	Elf_ed_2msb
#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_sh:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */
