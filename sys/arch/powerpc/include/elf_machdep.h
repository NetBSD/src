/*	$NetBSD: elf_machdep.h,v 1.2 1998/11/24 11:17:17 tsubai Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	Elf_ed_2msb
#define	ELF32_MACHDEP_ID_CASES						\
		case Elf_em_ppc:					\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#include <machine/reloc.h>		/* XXX */

#define R_TYPE(name) __CONCAT(RELOC_,name)
