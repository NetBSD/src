/*	$NetBSD: elf_machdep.h,v 1.1.1.1.2.1 2000/11/20 20:46:45 bouyer Exp $	*/

#include <mips/elf_machdep.h>

/*
 * HPCMIPS are mipsel machines
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
