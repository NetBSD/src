/*	$NetBSD: elf_machdep.h,v 1.1.12.2 2002/03/06 02:13:43 simonb Exp $	*/

#include <mips/elf_machdep.h>

#if defined(__MIPSEB__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#elif defined(__MIPSEL__)
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#else
#error neither __MIPSEL__ nor __MIPSEB__ are defined.
#endif
