/*	$NetBSD: elf_machdep.h,v 1.2.18.1 1999/11/12 11:47:27 nisimura Exp $	*/

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
