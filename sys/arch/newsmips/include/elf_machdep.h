/*	$NetBSD: elf_machdep.h,v 1.1.18.1 1999/12/27 18:33:09 wrstuden Exp $	*/

#include <mips/elf_machdep.h>

/*
 * newses are mipseb machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
