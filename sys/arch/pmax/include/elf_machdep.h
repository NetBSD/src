/*	$NetBSD: elf_machdep.h,v 1.3 1999/10/25 13:55:09 kleink Exp $	*/

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
