/*	$NetBSD: param.h,v 1.10.118.1 2008/03/24 07:15:02 keiichi Exp $	*/

#define MACHINE		"ofppc"
#include <powerpc/param.h>

/* at this offset we mmap() the PCI IO range in display drivers */
#define PCI_MAGIC_IO_RANGE      0xfeff0000
