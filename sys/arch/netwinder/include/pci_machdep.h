/* $NetBSD: pci_machdep.h,v 1.2.8.2 2002/10/18 02:39:04 nathanw Exp $ */

#ifndef _NETWINDER_PCI_MACHDEP_H_
#define	_NETWINDER_PCI_MACHDEP_H_

#include <arm/pci_machdep.h>

void		netwinder_pci_attach_hook __P((struct device *,
		    struct device *, struct pcibus_attach_args *));

#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH

#endif /* _NETWINDER_PCI_MACHDEP_H_ */
