/*	$NetBSD: elf_machdep.h,v 1.2.32.1 1999/11/15 00:39:00 fvdl Exp $	*/

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
