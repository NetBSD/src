/* $NetBSD: pci_machdep.h,v 1.3.120.1 2009/04/28 07:34:29 skrll Exp $ */

#ifndef _NETWINDER_PCI_MACHDEP_H_
#define	_NETWINDER_PCI_MACHDEP_H_

#include <arm/pci_machdep.h>

void		netwinder_pci_attach_hook(struct device *,
		    struct device *, struct pcibus_attach_args *);

#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH

#endif /* _NETWINDER_PCI_MACHDEP_H_ */
