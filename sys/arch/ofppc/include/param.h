/*	$NetBSD: param.h,v 1.11 2008/03/07 17:13:49 phx Exp $	*/

#define MACHINE		"ofppc"
#include <powerpc/param.h>

/* at this offset we mmap() the PCI IO range in display drivers */
#define PCI_MAGIC_IO_RANGE      0xfeff0000
