/*	$NetBSD: elf_machdep.h,v 1.1.1.1.4.1 1999/11/15 00:37:51 fvdl Exp $	*/

#include <mips/elf_machdep.h>

/*
 * HPCMIPS are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
