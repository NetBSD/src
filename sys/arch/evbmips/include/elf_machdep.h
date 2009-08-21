/*	$NetBSD: elf_machdep.h,v 1.2.74.1 2009/08/21 17:52:16 matt Exp $	*/

#include <mips/elf_machdep.h>

#if defined(__MIPSEB__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2MSB
#elif defined(__MIPSEL__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2LSB
#elif !defined(HAVE_NBTOOL_CONFIG_H)
#error neither __MIPSEL__ nor __MIPSEB__ are defined.
#endif
