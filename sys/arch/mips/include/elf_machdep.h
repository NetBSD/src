/*	$NetBSD: elf_machdep.h,v 1.2 1996/11/11 20:33:12 jonathan Exp $	*/

#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_mips:					\
			break;

#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */
/*
 * Tell the kernel ELF exec code not to try relocating the interpreter
 * (ld.so) for dynamically-linked ELF binaries
 */
#ifdef _KERNEL
#define ELF_INTERP_NON_RELOCATABLE
#endif
