/*	$NetBSD: elf_machdep.h,v 1.1.10.1 1997/10/14 09:09:59 thorpej Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	Elf_ed_2lsb
#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_386:					\
		case Elf_em_486:					\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */
