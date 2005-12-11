/*	$NetBSD: pci_machdep.h,v 1.2 2005/12/11 12:17:51 christos Exp $	*/

#include <arm/pci_machdep.h>
#define __HAVE_PCI_CONF_HOOK
int pci_conf_hook(void *, int, int, int, pcireg_t);

