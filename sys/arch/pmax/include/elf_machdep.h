/*	$NetBSD: elf_machdep.h,v 1.2.34.1 1999/12/27 18:33:27 wrstuden Exp $	*/

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
