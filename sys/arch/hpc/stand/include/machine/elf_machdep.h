/*	$NetBSD: elf_machdep.h,v 1.1.26.3 2004/09/21 13:15:59 skrll Exp $	*/

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
