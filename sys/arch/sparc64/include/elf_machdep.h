/*	$NetBSD: elf_machdep.h,v 1.1.1.1 1998/06/20 04:58:51 eeh Exp $	*/

#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_sparc:					\
		case Elf_em_sparc32plus:				\
			break;

#define	ELF64_MACHDEP_ID_CASES						\
		case Elf_em_sparc32plus:				\
			break;
