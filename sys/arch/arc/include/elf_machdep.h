/*	$NetBSD: elf_machdep.h,v 1.4 2000/01/31 15:51:35 soda Exp $	*/

#include <mips/elf_machdep.h>

/*
 * arc is mipsel platform
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
