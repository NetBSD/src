/*	$NetBSD: elf_machdep.h,v 1.2 1996/12/17 03:45:05 jonathan Exp $	*/

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	Elf_ed_2lsb

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
