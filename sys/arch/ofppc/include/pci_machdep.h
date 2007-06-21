/*	$NetBSD: pci_machdep.h,v 1.4.38.1 2007/06/21 18:49:43 garbled Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
