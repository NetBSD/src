/* $NetBSD: pci_machdep.h,v 1.3 2002/10/09 00:33:39 thorpej Exp $ */

#ifndef _NETWINDER_PCI_MACHDEP_H_
#define	_NETWINDER_PCI_MACHDEP_H_

#include <arm/pci_machdep.h>

void		netwinder_pci_attach_hook __P((struct device *,
		    struct device *, struct pcibus_attach_args *));

#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH

#endif /* _NETWINDER_PCI_MACHDEP_H_ */
