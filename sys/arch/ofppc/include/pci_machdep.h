/*	$NetBSD: pci_machdep.h,v 1.4.30.1 2007/10/23 20:13:40 ad Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);
