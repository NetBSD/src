/*	$NetBSD: elf_machdep.h,v 1.2 2006/11/25 07:32:53 riz Exp $	*/

#include <mips/elf_machdep.h>

#if defined(__MIPSEB__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#elif defined(__MIPSEL__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#elif !defined(HAVE_NBTOOL_CONFIG_H)
#error neither __MIPSEL__ nor __MIPSEB__ are defined.
#endif
