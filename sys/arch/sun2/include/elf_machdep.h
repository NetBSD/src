/*	$NetBSD: elf_machdep.h,v 1.3 2001/12/09 23:05:59 thorpej Exp $	*/

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
