/*	$NetBSD: elf_machdep.h,v 1.2 1997/10/11 19:11:10 christos Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	Elf_ed_2lsb
#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_386:					\
		case Elf_em_486:					\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */
