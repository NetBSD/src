/*	$NetBSD: elf_machdep.h,v 1.3 1999/10/25 13:55:09 kleink Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_PPC:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#include <machine/reloc.h>		/* XXX */

#define R_TYPE(name) __CONCAT(RELOC_,name)
