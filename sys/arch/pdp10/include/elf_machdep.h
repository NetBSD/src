/*	$NetBSD: elf_machdep.h,v 1.1 2003/08/19 10:53:05 ragge Exp $	*/

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
