/*	$NetBSD: elf_machdep.h,v 1.1.2.2 2001/04/21 17:54:36 bouyer Exp $	*/

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

#define ARCH_ELFSIZE		32	/* MD native binary size */
