/*	$NetBSD: elf_machdep.h,v 1.4.2.1 2002/01/10 19:48:39 thorpej Exp $	*/

#ifndef _BYTE_ORDER
#error Define _BYTE_ORDER!
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#else
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#endif
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_SH:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_SH

#define ARCH_ELFSIZE		32	/* MD native binary size */
