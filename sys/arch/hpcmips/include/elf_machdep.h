/*	$NetBSD: elf_machdep.h,v 1.1.1.1.6.1 1999/12/27 18:32:05 wrstuden Exp $	*/

#include <mips/elf_machdep.h>

/*
 * HPCMIPS are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
