/*	$NetBSD: elf_machdep.h,v 1.1.16.1 1999/11/15 00:38:48 fvdl Exp $	*/

#include <mips/elf_machdep.h>

/*
 * newses are mipseb machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
