/*	$NetBSD: elf_machdep.h,v 1.2.8.2 2002/01/08 00:28:09 nathanw Exp $	*/

#ifndef _MACHINE_ELF_MACHDEP_H_
#define _MACHINE_ELF_MACHDEP_H_

#include <m68k/elf_machdep.h>

#undef	ELF32_MACHDEP_ID_CASES
#define	ELF32_MACHDEP_ID_CASES						\
                case EM_68000:						\
                        break;

#undef	ELF32_MACHDEP_ID
#define	ELF32_MACHDEP_ID	EM_68000

#endif
