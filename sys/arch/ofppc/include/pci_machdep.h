/*	$NetBSD: pci_machdep.h,v 1.5.4.1 2007/12/26 19:42:33 ad Exp $	*/

#include <powerpc/pci_machdep.h>

int	pcidev_to_ofdev(pci_chipset_tag_t, pcitag_t);

#define __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#define pciide_machdep_compat_intr_establish				\
	genppc_pciide_machdep_compat_intr_establish
