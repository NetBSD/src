/*	$NetBSD: elf_machdep.h,v 1.2.30.1 2000/11/20 20:20:27 bouyer Exp $	*/

#ifndef _PMAX_ELF_MACHDEP_H_
#define _PMAX_ELF_MACHDEP_H_

#include <mips/elf_machdep.h>

/*
 * pmaxes are mipsel machines
 */
#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */

#endif	/* !_PMAX_ELF_MACHDEP_H_ */
