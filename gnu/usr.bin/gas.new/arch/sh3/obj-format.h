/*	$NetBSD: obj-format.h,v 1.1 1999/10/07 12:00:59 msaitoh Exp $	*/

#ifdef DEFAULT_ELF
# include "obj-elf.h"
#else
# include "obj-coff.h"
#endif
