/*	$NetBSD: elf_machdep.h,v 1.1 1996/09/26 21:50:58 cgd Exp $	*/

#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_386:					\
		case Elf_em_486:					\
			break;

#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */
