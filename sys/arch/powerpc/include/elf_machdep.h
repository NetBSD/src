/*	$NetBSD: elf_machdep.h,v 1.4.8.1 2002/01/10 19:48:02 thorpej Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_PPC:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_PPC

#define ARCH_ELFSIZE		32	/* MD native binary size */

#include <machine/reloc.h>		/* XXX */

#define R_TYPE(name) __CONCAT(RELOC_,name)
