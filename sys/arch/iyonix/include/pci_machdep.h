/*	$NetBSD: pci_machdep.h,v 1.1.2.2 2004/10/19 15:56:37 skrll Exp $	*/

#include <arm/pci_machdep.h>
#define __HAVE_PCI_CONF_HOOK
int pci_conf_hook(void *, int, int, int, pcireg_t);

