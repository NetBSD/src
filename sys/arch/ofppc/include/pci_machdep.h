/*	$NetBSD: pci_machdep.h,v 1.4.50.1 2007/11/06 23:19:59 matt Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
