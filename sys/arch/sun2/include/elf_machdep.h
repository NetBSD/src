/*	$NetBSD: elf_machdep.h,v 1.2 2001/06/27 19:20:22 fredette Exp $	*/

#ifndef _MACHINE_ELF_MACHDEP_H_
#define _MACHINE_ELF_MACHDEP_H_

#include <m68k/elf_machdep.h>

#undef	ELF32_MACHDEP_ID_CASES
#define	ELF32_MACHDEP_ID_CASES						\
                case EM_68000:						\
                        break;

#endif
