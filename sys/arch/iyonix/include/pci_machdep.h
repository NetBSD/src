/*	$NetBSD: pci_machdep.h,v 1.1 2004/10/13 23:28:36 gavan Exp $	*/

#include <arm/pci_machdep.h>
#define __HAVE_PCI_CONF_HOOK
int pci_conf_hook(void *, int, int, int, pcireg_t);

