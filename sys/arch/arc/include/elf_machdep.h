/*	$NetBSD: elf_machdep.h,v 1.4.6.2 2000/11/20 20:00:35 bouyer Exp $	*/

#include <mips/elf_machdep.h>

/*
 * arc is mipsel platform
 */

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
