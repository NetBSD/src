/*	$NetBSD: elf_machdep.h,v 1.1 1998/02/18 13:48:18 tsubai Exp $	*/

#include <mips/elf_machdep.h>

/*
 * newses are mipseb machines
 */

#define ELF32_MACHDEP_ENDIANNESS	Elf_ed_2msb

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
