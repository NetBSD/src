/*	$NetBSD: pci_machdep.h,v 1.5 2007/10/17 19:56:08 garbled Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
