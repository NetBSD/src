/*	$NetBSD: elf_machdep.h,v 1.1 2001/05/28 16:22:18 thorpej Exp $	*/

#ifndef _ALGOR_ELF_MACHDEP_H_
#define _ALGOR_ELF_MACHDEP_H_

#include <mips/elf_machdep.h>

/*
 * Algorithmics boards are mipsel machines by default
 */
#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */

#endif	/* !_ALGOR_ELF_MACHDEP_H_ */
