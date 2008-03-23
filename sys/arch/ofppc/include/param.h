/*	param.h,v 1.10 2001/10/20 08:27:12 billc Exp	*/

#define MACHINE		"ofppc"
#include <powerpc/param.h>

/* at this offset we mmap() the PCI IO range in display drivers */
#define PCI_MAGIC_IO_RANGE      0xfeff0000
