/*	$NetBSD: pci_machdep.h,v 1.4.54.1 2007/10/25 22:36:14 bouyer Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
