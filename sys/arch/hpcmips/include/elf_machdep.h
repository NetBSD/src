/*	$NetBSD: elf_machdep.h,v 1.1.1.1 1999/09/16 12:23:22 takemura Exp $	*/

#include <mips/elf_machdep.h>

/*
 * HPCMIPS are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	Elf_ed_2lsb

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
