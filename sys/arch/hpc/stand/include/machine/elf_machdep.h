/*	$NetBSD: elf_machdep.h,v 1.3 2005/12/11 12:17:29 christos Exp $	*/

/* Windows CE architecture */
#define	ELFSIZE		32

#ifdef MIPS
#include "../../../../hpcmips/include/elf_machdep.h"
#endif
#ifdef SHx
#include "../../../../hpcsh/include/elf_machdep.h"
#endif
#ifdef ARM
#include "../../../../arm/include/elf_machdep.h"
#endif
