/*	$NetBSD: pci_machdep.h,v 1.3.12.1 2007/10/27 11:27:27 yamt Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
