/*	$NetBSD: obj-format.h,v 1.1.4.1 1999/12/27 18:29:18 wrstuden Exp $	*/

#ifdef DEFAULT_ELF
# include "obj-elf.h"
#else
# include "obj-coff.h"
#endif
