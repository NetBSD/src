/*	$NetBSD: param.h,v 1.10.122.1 2008/04/03 12:42:22 mjf Exp $	*/

#define MACHINE		"ofppc"
#include <powerpc/param.h>

/* at this offset we mmap() the PCI IO range in display drivers */
#define PCI_MAGIC_IO_RANGE      0xfeff0000
