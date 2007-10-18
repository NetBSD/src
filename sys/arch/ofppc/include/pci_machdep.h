/*	$NetBSD: pci_machdep.h,v 1.4.52.1 2007/10/18 08:32:22 yamt Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
