/*	$NetBSD: elf_machdep.h,v 1.4 2000/01/09 15:34:42 ad Exp $	*/

#ifndef _PMAX_ELF_MACHDEP_H_
#define _PMAX_ELF_MACHDEP_H_

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */
#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */

#endif	/* !_PMAX_ELF_MACHDEP_H_ */
