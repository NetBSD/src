/*	$NetBSD: elf_machdep.h,v 1.1.14.1 2000/11/20 20:17:24 bouyer Exp $	*/

#include <mips/elf_machdep.h>

/*
 * newses are mipseb machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
