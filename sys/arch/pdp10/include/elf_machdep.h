/*	$NetBSD: elf_machdep.h,v 1.1.4.3 2004/09/18 14:38:42 skrll Exp $	*/

#define	ELF36_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF36_MACHDEP_ID_CASES						\
		case EM_PDP10:						\
			break;

#define	ELF32_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF32_MACHDEP_ID_CASES						\
		/* no 32-bit ELF machine types supported */

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF36_MACHDEP_ID	EM_PDP10

#define ARCH_ELFSIZE		36	/* MD native binary size XXX */
