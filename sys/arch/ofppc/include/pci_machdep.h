/*	$NetBSD: pci_machdep.h,v 1.4.48.1 2007/10/26 15:43:03 joerg Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
